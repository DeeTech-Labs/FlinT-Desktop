#include "DeviceSession.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QSaveFile>
#include <QTimeZone>
#include <QSerialPortInfo>
#include <QSet>
#include <QSettings>
#include <QRegularExpression>

DeviceSession::DeviceSession(QObject* parent, PortProvider portProvider)
    : QObject(parent), m_portProvider(std::move(portProvider))
{
    connect(this, &DeviceSession::stateChanged, this, &DeviceSession::viewStateChanged);
    connect(this, &DeviceSession::dataChanged, this, &DeviceSession::viewStateChanged);
    connect(&m_ble, &BleTransport::pairingCodeChanged, this, &DeviceSession::pairingCodeChanged);
    m_autoSyncTime = QSettings().value("connection/autoSyncTime",false).toBool();
    connect(&m_ble, &BleTransport::devicesChanged, this, &DeviceSession::portsChanged);
    connect(&m_ble, &BleTransport::received, this, &DeviceSession::acceptBytes);
    connect(&m_ble, &BleTransport::error, this, [this](const QString& error) {
        if (!m_closing) fail(error);
    });
    connect(&m_ble, &BleTransport::readyChanged, this, [this] {
        if (m_usingBle && m_ble.ready()) send(Command::Hello, "@HELLO\n");
    });
    m_port.setReadBufferSize(16384);
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(4000);
    m_poll.setInterval(2000);
    connect(&m_poll, &QTimer::timeout, this, &DeviceSession::refreshStatus);
    connect(&m_timeout, &QTimer::timeout, this, [this] {
        fail(tr("The device did not respond. Check the connection and FlinT OS version."));
    });
    connect(&m_port, &QSerialPort::readyRead, this, &DeviceSession::readAvailable);
    connect(&m_port, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
        if (!m_closing && error != QSerialPort::NoError && error != QSerialPort::NotOpenError
            && error != QSerialPort::UnsupportedOperationError)
            fail(m_port.errorString());
    });
    m_discovery.setInterval(1000);
    connect(&m_discovery, &QTimer::timeout, this, &DeviceSession::scanDevices);
    refreshPorts();
}

QString DeviceSession::formatTime(qint64 epoch, const QString& timezone, const QString& format) const
{
    const QTimeZone zone(timezone.toUtf8());
    if (!zone.isValid()) return tr("Unknown time zone");
    return QDateTime::fromSecsSinceEpoch(epoch, zone).toString(format);
}

QString DeviceSession::connectionState() const
{
    if (m_ready) return tr("Connected");
    if (m_simulating || m_port.isOpen() || m_ble.active()) return tr("Checking device…");
    if (m_autoConnect) return tr("Searching for device…");
    return tr("Disconnected");
}

QString DeviceSession::viewState() const
{
    if (connecting()) return QStringLiteral("Connecting");
    if (!m_error.isEmpty()) return QStringLiteral("Error");
    if (m_ready) return m_simulating ? QStringLiteral("Simulated") : QStringLiteral("Connected");
    if (!m_lastUpdated.isEmpty()) return QStringLiteral("StaleData");
    return QStringLiteral("Disconnected");
}

void DeviceSession::refreshPorts()
{
    QVariantList ports;
    if (m_portProvider) {
        ports = m_portProvider();
    } else {
        for (const auto& port : QSerialPortInfo::availablePorts()) {
            if (!port.hasVendorIdentifier() && port.serialNumber().isEmpty()) continue;
            ports.append(QVariantMap{{"path", port.systemLocation()},
                {"label", port.portName() + " — " + port.description()},
                // Spark's native USB Serial/JTAG. Other USB adapters remain available manually.
                {"automatic", port.hasVendorIdentifier() && port.hasProductIdentifier()
                    && port.vendorIdentifier() == 0x303a && port.productIdentifier() == 0x1001}});
        }
    }
    if (m_ports != ports) {
        m_ports = ports;
        emit portsChanged();
    }
}

void DeviceSession::setAutoSyncTime(bool enabled)
{
    m_autoSyncTime = enabled;
    QSettings().setValue("connection/autoSyncTime",enabled);
    emit stateChanged();
}

void DeviceSession::setAutoConnect(bool enabled)
{
    if ((enabled && m_simulating) || m_autoConnect == enabled) return;
    m_autoConnect = enabled;
    if (enabled) {
        m_retryAfter.clear();
        m_discovery.start();
        QTimer::singleShot(0, this, &DeviceSession::scanDevices);
    } else {
        m_discovery.stop();
    }
    emit stateChanged();
}

void DeviceSession::scanDevices()
{
    if (!m_autoConnect) return;
    if (m_usingBle) {
        if (!m_ble.active() && !m_lastBleId.isEmpty()
            && QDateTime::currentMSecsSinceEpoch() >= m_bleRetryAfter) {
            m_bleRetryAfter = QDateTime::currentMSecsSinceEpoch() + 15000;
            m_error.clear();
            m_ble.open(m_lastBleId);
            emit stateChanged();
        }
        return;
    }
    refreshPorts();
    QSet<QString> present;
    for (const auto& entry : m_ports) present.insert(entry.toMap().value("path").toString());
    for (auto it = m_retryAfter.begin(); it != m_retryAfter.end();) {
        if (!present.contains(it.key())) it = m_retryAfter.erase(it);
        else ++it;
    }
    if (m_port.isOpen()) {
        if (!present.contains(m_activePath)) closePort();
        else return;
    }
    const auto now = QDateTime::currentMSecsSinceEpoch();
    for (const auto& entry : m_ports) {
        const auto port = entry.toMap();
        const auto path = port.value("path").toString();
        if (path.isEmpty() || !port.value("automatic").toBool()
            || m_retryAfter.value(path) > now) continue;
        // Throttle failed/busy ports and give the other candidates a turn.
        m_retryAfter.insert(path, now + 15000);
        openPort(path);
        return;
    }
}

void DeviceSession::connectPort(const QString& path)
{
    openPort(path);
}

int DeviceSession::request(const QString& operation, const QString& arguments)
{
    if (!m_ready || busy() || !m_info.value("capabilities").toArray().contains("control")) return 0;
    if (!QRegularExpression("^[A-Z]{1,23}$").match(operation).hasMatch()
        || arguments.size() > 287 || !QRegularExpression("^[ -?A-~]*$").match(arguments).hasMatch()) return 0;
    m_requestId = m_nextRequest++;
    if (m_nextRequest > 999999999) m_nextRequest = 1;
    const auto frame = "@REQ " + QByteArray::number(m_requestId) + ' ' + operation.toLatin1()
        + (arguments.isEmpty() ? QByteArray{} : ' ' + arguments.toLatin1()) + '\n';
    const int id = m_requestId;
    send(Command::Request, frame);
    return m_pending == Command::Request ? id : 0;
}

void DeviceSession::startSimulator()
{
    setAutoConnect(false);
    closePort();
    m_simulating = true;
    m_usingBle = false;
    m_error.clear(); m_notification.clear();
    send(Command::Hello, "@HELLO\n");
}

void DeviceSession::connectBle(const QString& id)
{
    setAutoConnect(false);
    closePort();
    m_simulating = false;
    m_usingBle = true;
    m_lastBleId = id;
    m_bleRetryAfter = QDateTime::currentMSecsSinceEpoch() + 15000;
    m_error.clear();
    m_notification.clear();
    setAutoConnect(true);
    m_ble.open(id);
    emit stateChanged();
}

void DeviceSession::openPort(const QString& path)
{
    closePort();
    m_simulating = false;
    m_usingBle = false;
    m_activePath = path;
    m_error.clear();
    m_notification.clear();
    if (path.isEmpty()) { fail(tr("Select the device's USB port.")); return; }
    m_port.setPortName(path);
    m_port.setBaudRate(115200);
    m_port.setFlowControl(QSerialPort::NoFlowControl);
    if (!m_port.open(QIODevice::ReadWrite)) {
        if (m_error.isEmpty()) fail(m_port.errorString());
        return;
    }
    // Release RTS before DTR, as IDF monitor does for native USB Serial/JTAG.
    m_port.setRequestToSend(false);
    m_port.setDataTerminalReady(false);
    m_port.clearError();
    appendLog(tr("Connecting: %1").arg(path));
    send(Command::Hello, "@HELLO\n");
    // Native USB can drop the first transfer while the host opens its endpoint.
    // Retry only the read-only handshake; never repeat settings or private writes.
    QTimer::singleShot(300, this, [this, path] {
        if (m_port.isOpen() && m_activePath == path && !m_usingBle && !m_simulating && m_pending == Command::Hello)
            m_port.write("@HELLO\n");
    });
}

void DeviceSession::disconnectDevice()
{
    setAutoConnect(false);
    m_notification = tr("Device disconnected.");
    closePort();
}

void DeviceSession::closePort()
{
    m_timeout.stop();
    m_poll.stop();
    m_pending = Command::None;
    m_ready = false;
    m_simulating = false;
    m_closing = true;
    m_ble.close();
    m_port.close();
    m_closing = false;
    m_rx.clear();
    m_discardLine = false;
    emit stateChanged();
}

void DeviceSession::fail(const QString& message)
{
    const int failedRequest = m_pending == Command::Request ? m_requestId : 0;
    m_error = message;
    closePort();
    if (failedRequest) emit requestFinished(failedRequest, false, {}, message);
    appendLog(tr("Error: %1").arg(message));
    emit failed(message);
}

void DeviceSession::appendLog(const QString& line)
{
    m_log.append(line.left(1024));
    while (m_log.size() > 150) m_log.removeFirst();
    emit logChanged();
}

void DeviceSession::clearLog()
{
    m_log.clear();
    emit logChanged();
}

void DeviceSession::send(Command command, const QByteArray& frame)
{
    if ((!m_simulating && (m_usingBle ? !m_ble.ready() : !m_port.isOpen())) || busy()) return;
    m_pending = command;
    m_timeout.start(m_usingBle ? (command == Command::Hello ? 30000 : 8000) : 4000);
    emit stateChanged();
    appendLog(QStringLiteral("> ") + QString::fromLatin1(frame).trimmed());
    if (m_simulating) {
        const auto reply = m_simulator.exchange(frame);
        QTimer::singleShot(20, this, [this, reply] { if (m_simulating) acceptBytes(reply); });
        return;
    }
    if (m_usingBle ? !m_ble.write(frame) : m_port.write(frame) != frame.size())
        fail(tr("Could not send the command."));
}

void DeviceSession::finishCommand()
{
    m_timeout.stop();
    m_pending = Command::None;
    emit stateChanged();
}

void DeviceSession::refreshStatus()
{
    if (m_ready && !busy()) send(Command::Status, "@STATUS\n");
}

void DeviceSession::syncTime()
{
    if (!m_ready) return;
    if (busy()) { m_syncOnConnect = true; return; }
    if (!m_info.value("capabilities").toArray().contains("time_sync")) return;
    m_sentEpoch = QDateTime::currentSecsSinceEpoch();
    send(Command::Time, "@TIME " + QByteArray::number(m_sentEpoch) + '\n');
}

void DeviceSession::readAvailable()
{
    acceptBytes(m_port.readAll());
}

void DeviceSession::acceptBytes(const QByteArray& bytes)
{
    for (char c : bytes) {
        if (c == '\n') {
            if (!m_discardLine) handleLine(m_rx.trimmed());
            m_rx.clear();
            m_discardLine = false;
        } else if (!m_discardLine) {
            m_rx.append(c);
            if (m_rx.size() > 8192) { m_rx.clear(); m_discardLine = true; }
        }
    }
}

void DeviceSession::handleLine(const QByteArray& line)
{
    if (line.isEmpty()) return;
    appendLog(QString::fromUtf8(line));
    if (m_pending == Command::None) return;
    if (m_pending == Command::Request && line.startsWith("FLINT RESULT ")) {
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(line.mid(13), &parseError);
        const auto response = document.object();
        if (parseError.error != QJsonParseError::NoError || !document.isObject()
            || !response.value("ok").isBool() || !response.value("id").isDouble()) {
            fail(tr("The device returned invalid JSON.")); return;
        }
        if (response.value("id").toInt() != m_requestId) return;
        const bool okay = response.value("ok").toBool();
        if (okay ? !response.contains("result") : !response.value("error").isString()) {
            fail(tr("The device returned invalid JSON.")); return;
        }
        const int id = m_requestId;
        finishCommand();
        emit requestFinished(id, okay, response.value("result").toVariant(), response.value("error").toString());
        return;
    }
    if (line.startsWith("FLINT ERROR ")) {
        fail(tr("FlinT OS: %1. Data collection requires firmware with INFO/STATUS support.")
                 .arg(QString::fromUtf8(line.mid(12))));
        return;
    }
    if (m_pending == Command::Hello && line.startsWith("FLINT HELLO ")) {
        if (!line.startsWith("FLINT HELLO 1 ")) {
            fail(tr("Unsupported companion protocol version."));
            return;
        }
        finishCommand();
        send(Command::Info, "@INFO\n");
        return;
    }
    if (m_pending == Command::Time && line.startsWith("FLINT TIME OK ")) {
        bool valid = false;
        const auto epoch = line.mid(14).toLongLong(&valid);
        if (!valid || epoch != m_sentEpoch) { fail(tr("Invalid time acknowledgement.")); return; }
        m_lastSync = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        emit dataChanged();
        m_notification = tr("Time synchronized with the PC.");
        finishCommand();
        refreshStatus();
        return;
    }
    QByteArray prefix;
    if (m_pending == Command::Info) prefix = "FLINT INFO ";
    else if (m_pending == Command::Status) prefix = "FLINT STATUS ";
    else return;
    if (!line.startsWith(prefix)) return; // Firmware diagnostic logs share the port.
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(line.mid(prefix.size()), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        fail(tr("The device returned invalid JSON."));
        return;
    }
    const auto data = document.object();
    if (m_pending == Command::Info) {
        bool valid = data.value("protocol").toInt() == 1 && data.value("capabilities").isArray();
        for (const auto* key : {"id", "model", "board", "chip", "firmware"})
            valid = valid && data.value(key).isString() && !data.value(key).toString().isEmpty();
        const auto capabilities = data.value("capabilities").toArray();
        valid = valid && capabilities.contains("info") && capabilities.contains("status");
        if (!valid) { fail(tr("Unsupported device description.")); return; }
        m_status = {};
        m_samples = {};
        m_lastUpdated.clear();
        m_info = data;
        m_lastSync.clear();
        m_syncOnConnect = m_autoSyncTime;
        m_ready = true;
        m_notification = tr("Device connected via %1.").arg(transport());
        finishCommand();
        emit dataChanged();
        m_poll.start();
        refreshStatus();
    } else {
        const bool validTime = data.value("time_valid").toBool();
        const auto epoch = data.value("epoch");
        if (!data.value("time_valid").isBool() || !data.value("pc_synced").isBool()
            || !data.value("usb_connected").isBool()
            || !data.value("uptime_ms").isDouble() || data.value("uptime_ms").toDouble() < 0
            || !data.value("free_heap_bytes").isDouble() || data.value("free_heap_bytes").toDouble() < 0
            || !data.value("timezone").isString() || !data.value("active_face").isString()
            || !data.value("settings").isObject()
            || (validTime ? !epoch.isDouble() || epoch.toDouble() < 1700000000
                          || epoch.toDouble() > 4102444799.0 : !epoch.isNull())) {
            fail(tr("Incomplete or invalid device status."));
            return;
        }
        m_status = data;
        m_clockDifference = validTime ? data.value("epoch").toInteger() - QDateTime::currentSecsSinceEpoch() : 0;
        m_lastUpdated = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
        m_samples.append(QJsonObject{{"received_at", m_lastUpdated}, {"status", data}});
        while (m_samples.size() > 600) m_samples.removeAt(0);
        finishCommand();
        emit dataChanged();
        emit sampleReceived();
        if (m_syncOnConnect && !busy()) { m_syncOnConnect = false; syncTime(); }
    }
}

QJsonObject DeviceSession::capture() const
{
    return {{"schema", "flint.desktop.capture/1"},
            {"exported_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
            {"device", m_info}, {"latest_status", m_status}, {"samples", m_samples}};
}

bool DeviceSession::saveData(const QUrl& destination)
{
    if (m_samples.isEmpty()) return false;
    QSaveFile file(destination.toLocalFile());
    const auto bytes = QJsonDocument(capture()).toJson(QJsonDocument::Indented);
    if (!destination.isLocalFile() || !file.open(QIODevice::WriteOnly)
        || file.write(bytes) != bytes.size() || !file.commit()) {
        m_error = tr("Could not save data: %1").arg(file.errorString());
        emit stateChanged();
        return false;
    }
    m_error.clear();
    m_notification = tr("Data saved: %1").arg(destination.toLocalFile());
    emit stateChanged();
    return true;
}
