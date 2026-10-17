#include "DeviceSession.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QSaveFile>
#include <QTimeZone>
#include <QSerialPortInfo>
#include <QSet>

DeviceSession::DeviceSession(QObject* parent, PortProvider portProvider)
    : QObject(parent), m_portProvider(std::move(portProvider))
{
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
    if (m_port.isOpen()) return tr("Checking device…");
    if (m_autoConnect) return tr("Searching for device…");
    return tr("Disconnected");
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

void DeviceSession::setAutoConnect(bool enabled)
{
    if (m_autoConnect == enabled) return;
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

void DeviceSession::openPort(const QString& path)
{
    closePort();
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
}

void DeviceSession::disconnectDevice()
{
    setAutoConnect(false);
    closePort();
}

void DeviceSession::closePort()
{
    m_timeout.stop();
    m_poll.stop();
    m_pending = Command::None;
    m_ready = false;
    m_closing = true;
    m_port.close();
    m_closing = false;
    m_rx.clear();
    m_discardLine = false;
    emit stateChanged();
}

void DeviceSession::fail(const QString& message)
{
    m_error = message;
    closePort();
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
    if (!m_port.isOpen() || busy()) return;
    m_pending = command;
    m_timeout.start();
    emit stateChanged();
    appendLog(QStringLiteral("> ") + QString::fromLatin1(frame).trimmed());
    if (m_port.write(frame) != frame.size()) fail(tr("Could not send the command."));
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
    if (!m_ready || busy()) return;
    if (!m_info.value("capabilities").toArray().contains("time_sync")) return;
    m_sentEpoch = QDateTime::currentSecsSinceEpoch();
    send(Command::Time, "@TIME " + QByteArray::number(m_sentEpoch) + '\n');
}

void DeviceSession::readAvailable()
{
    const auto bytes = m_port.readAll();
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
    if (line.startsWith("FLINT ERROR ")) {
        fail(tr("FlinT OS: %1. Data collection requires firmware with INFO/STATUS support.")
                 .arg(QString::fromUtf8(line.mid(12))));
        return;
    }
    if (m_pending == Command::Hello && line.startsWith("FLINT HELLO ")) {
        if (!line.startsWith("FLINT HELLO 1 ")) {
            fail(tr("Unsupported USB protocol version."));
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
        m_ready = true;
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
        m_lastUpdated = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
        m_samples.append(QJsonObject{{"received_at", m_lastUpdated}, {"status", data}});
        while (m_samples.size() > 600) m_samples.removeAt(0);
        finishCommand();
        emit dataChanged();
        emit sampleReceived();
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
