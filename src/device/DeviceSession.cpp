#include "DeviceSession.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QSaveFile>
#include <QSerialPortInfo>

DeviceSession::DeviceSession(QObject* parent) : QObject(parent)
{
    m_port.setReadBufferSize(16384);
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(4000);
    m_poll.setInterval(2000);
    connect(&m_poll, &QTimer::timeout, this, &DeviceSession::refreshStatus);
    connect(&m_timeout, &QTimer::timeout, this, [this] {
        fail(tr("Устройство не ответило. Проверьте подключение и версию FlinT OS."));
    });
    connect(&m_port, &QSerialPort::readyRead, this, &DeviceSession::readAvailable);
    connect(&m_port, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
        if (!m_closing && error != QSerialPort::NoError && error != QSerialPort::NotOpenError
            && error != QSerialPort::UnsupportedOperationError)
            fail(m_port.errorString());
    });
    refreshPorts();
}

QString DeviceSession::connectionState() const
{
    if (m_ready) return tr("Подключено");
    if (m_port.isOpen()) return tr("Проверка устройства…");
    return tr("Отключено");
}

void DeviceSession::refreshPorts()
{
    m_ports.clear();
    for (const auto& port : QSerialPortInfo::availablePorts()) {
        if (!port.hasVendorIdentifier() && port.serialNumber().isEmpty()) continue;
        m_ports.append(QVariantMap{{"path", port.systemLocation()},
                                  {"label", port.portName() + " — " + port.description()}});
    }
    emit portsChanged();
}

void DeviceSession::connectPort(const QString& path)
{
    disconnectDevice();
    m_error.clear();
    m_notification.clear();
    m_info = {};
    m_status = {};
    m_samples = {};
    m_lastUpdated.clear();
    emit dataChanged();
    if (path.isEmpty()) { fail(tr("Выберите USB-порт устройства.")); return; }
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
    appendLog(tr("Подключение: %1").arg(path));
    send(Command::Hello, "@HELLO\n");
}

void DeviceSession::disconnectDevice()
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
    disconnectDevice();
    appendLog(tr("Ошибка: %1").arg(message));
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
    if (m_port.write(frame) != frame.size()) fail(tr("Не удалось отправить команду."));
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
        fail(tr("FlinT OS: %1. Для сбора данных нужна прошивка с INFO/STATUS.")
                 .arg(QString::fromUtf8(line.mid(12))));
        return;
    }
    if (m_pending == Command::Hello && line.startsWith("FLINT HELLO ")) {
        if (!line.startsWith("FLINT HELLO 1 ")) {
            fail(tr("Неподдерживаемая версия USB-протокола."));
            return;
        }
        finishCommand();
        send(Command::Info, "@INFO\n");
        return;
    }
    if (m_pending == Command::Time && line.startsWith("FLINT TIME OK ")) {
        bool valid = false;
        const auto epoch = line.mid(14).toLongLong(&valid);
        if (!valid || epoch != m_sentEpoch) { fail(tr("Неверное подтверждение времени.")); return; }
        m_notification = tr("Время синхронизировано с ПК.");
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
        fail(tr("Устройство прислало некорректный JSON."));
        return;
    }
    const auto data = document.object();
    if (m_pending == Command::Info) {
        bool valid = data.value("protocol").toInt() == 1 && data.value("capabilities").isArray();
        for (const auto* key : {"id", "model", "board", "chip", "firmware"})
            valid = valid && data.value(key).isString() && !data.value(key).toString().isEmpty();
        const auto capabilities = data.value("capabilities").toArray();
        valid = valid && capabilities.contains("info") && capabilities.contains("status");
        if (!valid) { fail(tr("Неподдерживаемое описание устройства.")); return; }
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
            fail(tr("Неполное или некорректное состояние устройства."));
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
        m_error = tr("Не удалось сохранить данные: %1").arg(file.errorString());
        emit stateChanged();
        return false;
    }
    m_error.clear();
    m_notification = tr("Данные сохранены: %1").arg(destination.toLocalFile());
    emit stateChanged();
    return true;
}
