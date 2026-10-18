#include "BleTransport.h"
#include <QLowEnergyDescriptor>

namespace {
const QBluetoothUuid serviceUuid(QStringLiteral("e7f10001-7a1e-4b5c-9d2f-46544c494e54"));
const QBluetoothUuid rxUuid(QStringLiteral("e7f10002-7a1e-4b5c-9d2f-46544c494e54"));
const QBluetoothUuid txUuid(QStringLiteral("e7f10003-7a1e-4b5c-9d2f-46544c494e54"));
QString key(const QBluetoothDeviceInfo& device) {
    return device.deviceUuid().isNull() ? device.address().toString() : device.deviceUuid().toString();
}
}
BleTransport::BleTransport(QObject* parent) : QObject(parent) {
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(30000);
    connect(&m_timeout, &QTimer::timeout, this, [this] { emit error(tr("Bluetooth connection timed out.")); });
}
void BleTransport::ensureDiscovery() {
    if (m_discovery) return;
    m_discovery = new QBluetoothDeviceDiscoveryAgent(this);
    m_discovery->setLowEnergyDiscoveryTimeout(12000);
    const auto rememberDevice = [this](const QBluetoothDeviceInfo& info) {
        if (!isCandidate(info)) return;
        for (auto& device : m_devices) {
            if (key(device) == key(info)) { device = info; emit devicesChanged(); return; }
        }
        m_devices.append(info);
        emit devicesChanged();
    };
    connect(m_discovery, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, rememberDevice);
    connect(m_discovery, &QBluetoothDeviceDiscoveryAgent::deviceUpdated, this,
            [rememberDevice](const QBluetoothDeviceInfo& info, QBluetoothDeviceInfo::Fields) { rememberDevice(info); });
    connect(m_discovery, &QBluetoothDeviceDiscoveryAgent::finished, this, [this,rememberDevice] {
        for (const auto& device : m_discovery->discoveredDevices()) rememberDevice(device);
        m_bluezDiscovery.stop();
        emit devicesChanged();
    });
    connect(m_discovery, &QBluetoothDeviceDiscoveryAgent::canceled, this, [this] { m_bluezDiscovery.stop(); emit devicesChanged(); });
    connect(m_discovery, &QBluetoothDeviceDiscoveryAgent::errorOccurred, this,
            [this] { m_bluezDiscovery.stop(); emit devicesChanged(); emit error(m_discovery->errorString()); });
}
bool BleTransport::isCandidate(const QBluetoothDeviceInfo& info) {
    const auto services = info.serviceUuids();
    // BlueZ may deliver the scan-response name before its service UUID list.
    // Named candidates still pass service/characteristic validation on connection.
    const bool namedCandidate = services.isEmpty() && info.name() == QStringLiteral("FlinT Spark");
    return (info.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration)
        && (services.contains(serviceUuid) || namedCandidate);
}
QVariantList BleTransport::devices() const {
    QVariantList result;
    for (const auto& device : m_devices)
        result.append(QVariantMap{{"path", key(device)}, {"label", device.name() + " · " + key(device)}});
    return result;
}
void BleTransport::scan() {
    if (active() || scanning()) return;
    ensureDiscovery();
    m_devices.clear();
    m_bluezDiscovery.start();
    m_discovery->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    emit devicesChanged();
}
void BleTransport::open(const QString& id) {
    close();
    for (const auto& device : m_devices) {
        if (key(device) != id) continue;
        if (m_discovery) m_discovery->stop();
        m_controller = QLowEnergyController::createCentral(device, this);
#ifdef Q_OS_LINUX
        // FlinT firmware advertises its public ESP32 identity (privacy disabled).
        // Without CAP_NET_ADMIN, Qt cannot infer this and defaults to RandomAddress.
        m_controller->setRemoteAddressType(QLowEnergyController::PublicAddress);
#endif
        connect(m_controller, &QLowEnergyController::connected, this, [this] { m_controller->discoverServices(); });
        connect(m_controller, &QLowEnergyController::discoveryFinished, this, &BleTransport::discoverService);
        connect(m_controller, &QLowEnergyController::disconnected, this, [this] { emit error(tr("Bluetooth device disconnected.")); });
        connect(m_controller, &QLowEnergyController::errorOccurred, this, [this] { emit error(m_controller->errorString()); });
        m_timeout.start();
#ifdef Q_OS_LINUX
        // BlueZ needs an explicit pairing request/agent before encrypted GATT access.
        if (!m_pairingAgent.start(device.address())) {
            emit error(tr("Bluetooth pairing failed. Accept the system pairing request and try again.")); return;
        }
        m_pairer = new QBluetoothLocalDevice(this);
        connect(m_pairer,&QBluetoothLocalDevice::pairingFinished,this,
                [this,address=device.address()](const QBluetoothAddress& peer,QBluetoothLocalDevice::Pairing pairing) {
            if (peer != address || !m_controller) return;
            if (pairing == QBluetoothLocalDevice::Paired || pairing == QBluetoothLocalDevice::AuthorizedPaired)
                m_controller->connectToDevice();
        });
        connect(m_pairer,&QBluetoothLocalDevice::errorOccurred,this,[this](QBluetoothLocalDevice::Error) {
            emit error(tr("Bluetooth pairing failed. Accept the system pairing request and try again."));
        });
        m_pairer->requestPairing(device.address(),QBluetoothLocalDevice::Paired);
#else
        m_controller->connectToDevice();
#endif
        return;
    }
    emit error(tr("Scan again and select a FlinT Bluetooth device."));
}
void BleTransport::discoverService() {
    m_service = m_controller->createServiceObject(serviceUuid, this);
    if (!m_service) { emit error(tr("FlinT Bluetooth service is missing.")); return; }
    connect(m_service, &QLowEnergyService::stateChanged, this, [this](QLowEnergyService::ServiceState state) {
        if (state == QLowEnergyService::RemoteServiceDiscovered) subscribe();
    });
    connect(m_service, &QLowEnergyService::errorOccurred, this, [this] { emit error(tr("Bluetooth service exchange failed.")); });
    connect(m_service, &QLowEnergyService::characteristicChanged, this,
            [this](const QLowEnergyCharacteristic& characteristic, const QByteArray& bytes) {
        if (characteristic == m_tx) emit received(bytes);
    });
    connect(m_service, &QLowEnergyService::descriptorWritten, this,
            [this](const QLowEnergyDescriptor& descriptor, const QByteArray& value) {
        if (descriptor == m_tx.clientCharacteristicConfiguration() && value == QByteArray::fromHex("0200")) {
            m_ready = true;
            m_timeout.stop();
            emit readyChanged();
        }
    });
    connect(m_service, &QLowEnergyService::characteristicWritten, this,
            [this](const QLowEnergyCharacteristic& characteristic, const QByteArray&) {
        if (characteristic == m_rx) { m_writeInFlight = false; writeNext(); }
    });
    m_service->discoverDetails();
}
void BleTransport::subscribe() {
    m_rx = m_service->characteristic(rxUuid);
    m_tx = m_service->characteristic(txUuid);
    const auto descriptor = m_tx.clientCharacteristicConfiguration();
    if (!m_rx.isValid() || !(m_rx.properties() & QLowEnergyCharacteristic::Write)
        || !m_tx.isValid() || !(m_tx.properties() & QLowEnergyCharacteristic::Indicate)
        || !descriptor.isValid()) {
        emit error(tr("Incompatible FlinT Bluetooth characteristics.")); return;
    }
    m_service->writeDescriptor(descriptor, QByteArray::fromHex("0200"));
}
bool BleTransport::write(const QByteArray& frame) {
    if (!m_ready || !m_service || frame.size() > 352 || !m_outgoing.isEmpty()) return false;
    m_outgoing = frame;
    writeNext();
    return true;
}
void BleTransport::writeNext() {
    if (m_writeInFlight || m_outgoing.isEmpty() || !m_ready || !m_service) return;
    const auto chunk = m_outgoing.left(20);
    m_outgoing.remove(0, chunk.size());
    m_writeInFlight = true;
    m_service->writeCharacteristic(m_rx, chunk, QLowEnergyService::WriteWithResponse);
}
void BleTransport::close() {
    m_outgoing.clear();
    m_writeInFlight = false;
    if (m_discovery) m_discovery->stop();
    m_timeout.stop();
    m_bluezDiscovery.stop();
    m_ready = false;
    if (m_pairer) { m_pairer->disconnect(this); m_pairer->deleteLater(); m_pairer = nullptr; }
    m_pairingAgent.stop();
    if (m_service) { m_service->disconnect(this); m_service->deleteLater(); m_service = nullptr; }
    if (m_controller) {
        auto* controller = m_controller;
        m_controller = nullptr;
        controller->disconnect(this);
        connect(controller,&QLowEnergyController::disconnected,controller,&QObject::deleteLater);
        controller->disconnectFromDevice();
        // Flush Linux's disconnect before application shutdown can discard the
        // asynchronous Qt request and leave the one-central peripheral occupied.
        m_bluezDiscovery.disconnectPeer(controller->remoteAddress());
        if (controller->state() == QLowEnergyController::UnconnectedState) controller->deleteLater();
        else QTimer::singleShot(3000,controller,&QObject::deleteLater);
    }
    m_rx = {}; m_tx = {};
}
