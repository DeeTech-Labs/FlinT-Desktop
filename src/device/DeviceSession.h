#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QUrl>
#include <QVariantList>

class DeviceSession : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList ports READ ports NOTIFY portsChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(QString connectionState READ connectionState NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)
    Q_PROPERTY(QString notification READ notification NOTIFY stateChanged)
    Q_PROPERTY(QVariantMap info READ info NOTIFY dataChanged)
    Q_PROPERTY(QVariantMap status READ status NOTIFY dataChanged)
    Q_PROPERTY(QString lastUpdated READ lastUpdated NOTIFY dataChanged)
    Q_PROPERTY(int sampleCount READ sampleCount NOTIFY dataChanged)
    Q_PROPERTY(QString log READ log NOTIFY logChanged)

public:
    explicit DeviceSession(QObject* parent = nullptr);
    QVariantList ports() const { return m_ports; }
    bool connected() const { return m_ready; }
    bool busy() const { return m_pending != Command::None; }
    QString connectionState() const;
    QString error() const { return m_error; }
    QString notification() const { return m_notification; }
    QVariantMap info() const { return m_info.toVariantMap(); }
    QVariantMap status() const { return m_status.toVariantMap(); }
    QString lastUpdated() const { return m_lastUpdated; }
    int sampleCount() const { return m_samples.size(); }
    QString log() const { return m_log.join('\n'); }
    QJsonObject capture() const;

    Q_INVOKABLE void refreshPorts();
    Q_INVOKABLE void connectPort(const QString& path);
    Q_INVOKABLE void disconnectDevice();
    Q_INVOKABLE void refreshStatus();
    Q_INVOKABLE void syncTime();
    Q_INVOKABLE bool saveData(const QUrl& destination);
    Q_INVOKABLE void clearLog();

signals:
    void portsChanged();
    void stateChanged();
    void dataChanged();
    void logChanged();
    void sampleReceived();
    void failed(const QString& message);

private:
    enum class Command { None, Hello, Info, Status, Time };
    void send(Command command, const QByteArray& frame);
    void readAvailable();
    void handleLine(const QByteArray& line);
    void fail(const QString& message);
    void appendLog(const QString& line);
    void finishCommand();
    QSerialPort m_port;
    QTimer m_timeout;
    QTimer m_poll;
    QVariantList m_ports;
    QJsonObject m_info;
    QJsonObject m_status;
    QJsonArray m_samples;
    QStringList m_log;
    QByteArray m_rx;
    QString m_error;
    QString m_notification;
    QString m_lastUpdated;
    Command m_pending = Command::None;
    qint64 m_sentEpoch = 0;
    bool m_ready = false;
    bool m_closing = false;
    bool m_discardLine = false;
};
