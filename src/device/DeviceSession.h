#pragma once

#include "BleTransport.h"
#include "SimulatedDevice.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QHash>
#include <functional>

class DeviceSession : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString pairingCode READ pairingCode NOTIFY pairingCodeChanged)
    Q_PROPERTY(QString viewState READ viewState NOTIFY viewStateChanged)
    Q_PROPERTY(QVariantList bleDevices READ bleDevices NOTIFY portsChanged)
    Q_PROPERTY(bool bleScanning READ bleScanning NOTIFY portsChanged)
    Q_PROPERTY(QString transport READ transport NOTIFY stateChanged)
    Q_PROPERTY(QVariantList ports READ ports NOTIFY portsChanged)
    Q_PROPERTY(bool autoSyncTime READ autoSyncTime WRITE setAutoSyncTime NOTIFY stateChanged)
    Q_PROPERTY(QString lastSync READ lastSync NOTIFY dataChanged)
    Q_PROPERTY(qint64 clockDifference READ clockDifference NOTIFY dataChanged)
    Q_PROPERTY(bool autoConnect READ autoConnect WRITE setAutoConnect NOTIFY stateChanged)
    Q_PROPERTY(bool connecting READ connecting NOTIFY stateChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(bool polling READ polling NOTIFY stateChanged)
    Q_PROPERTY(QString connectionState READ connectionState NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)
    Q_PROPERTY(QString notification READ notification NOTIFY stateChanged)
    Q_PROPERTY(QVariantMap info READ info NOTIFY dataChanged)
    Q_PROPERTY(QVariantMap status READ status NOTIFY dataChanged)
    Q_PROPERTY(QString lastUpdated READ lastUpdated NOTIFY dataChanged)
    Q_PROPERTY(int sampleCount READ sampleCount NOTIFY dataChanged)
    Q_PROPERTY(QString log READ log NOTIFY logChanged)

public:
    QVariantList bleDevices() const { return m_ble.devices(); }
    bool bleScanning() const { return m_ble.scanning(); }
    QString transport() const { return m_simulating ? QStringLiteral("SIMULATOR") : m_usingBle ? QStringLiteral("BLE") : QStringLiteral("USB"); }
    Q_INVOKABLE int request(const QString& operation, const QString& arguments = {});
    Q_INVOKABLE void startSimulator();
    Q_INVOKABLE void scanBle() { m_ble.scan(); }
    Q_INVOKABLE void connectBle(const QString& id);
    using PortProvider = std::function<QVariantList()>;
    explicit DeviceSession(QObject* parent = nullptr, PortProvider portProvider = {});
    bool autoSyncTime() const { return m_autoSyncTime; }
    void setAutoSyncTime(bool enabled);
    QString lastSync() const { return m_lastSync; }
    qint64 clockDifference() const { return m_clockDifference; }
    bool autoConnect() const { return m_autoConnect; }
    void setAutoConnect(bool enabled);
    QVariantList ports() const { return m_ports; }
    bool connecting() const { return !m_ready && (m_port.isOpen() || m_ble.active()); }
    bool connected() const { return m_ready; }
    bool busy() const { return m_pending != Command::None; }
    bool polling() const { return m_pending == Command::Status; }
    QString connectionState() const;
    QString viewState() const;
    QString pairingCode() const { return m_ble.pairingCode(); }
    Q_INVOKABLE void confirmPairing(bool accepted) { m_ble.confirmPairing(accepted); }
    QString error() const { return m_error; }
    QString notification() const { return m_notification; }
    QVariantMap info() const { return m_info.toVariantMap(); }
    QVariantMap status() const { return m_status.toVariantMap(); }
    QString lastUpdated() const { return m_lastUpdated; }
    int sampleCount() const { return m_samples.size(); }
    QString log() const { return m_log.join('\n'); }
    QJsonObject capture() const;

    Q_INVOKABLE QString formatTime(qint64 epoch, const QString& timezone, const QString& format) const;
    Q_INVOKABLE void refreshPorts();
    Q_INVOKABLE void connectPort(const QString& path);
    Q_INVOKABLE void disconnectDevice();
    Q_INVOKABLE void refreshStatus();
    Q_INVOKABLE void syncTime();
    Q_INVOKABLE bool saveData(const QUrl& destination);
    Q_INVOKABLE void clearLog();

signals:
    void viewStateChanged();
    void pairingCodeChanged();
    void requestFinished(int id, bool okay, const QVariant& result, const QString& error);
    void portsChanged();
    void stateChanged();
    void dataChanged();
    void logChanged();
    void sampleReceived();
    void failed(const QString& message);

private:
    enum class Command { None, Hello, Info, Status, Time, Request };
    void scanDevices();
    void closePort();
    void openPort(const QString& path);
    void send(Command command, const QByteArray& frame);
    void readAvailable();
    void acceptBytes(const QByteArray& bytes);
    void handleLine(const QByteArray& line);
    void fail(const QString& message);
    void appendLog(const QString& line);
    void finishCommand();
    SimulatedDevice m_simulator;
    bool m_simulating = false;
    int m_nextRequest = 1;
    int m_requestId = 0;
    BleTransport m_ble;
    bool m_usingBle = false;
    QString m_lastBleId;
    qint64 m_bleRetryAfter = 0;
    QSerialPort m_port;
    QTimer m_timeout;
    QTimer m_poll;
    QTimer m_discovery;
    PortProvider m_portProvider;
    QHash<QString, qint64> m_retryAfter;
    QString m_activePath;
    bool m_autoConnect = false;
    bool m_autoSyncTime = false;
    bool m_syncOnConnect = false;
    QString m_lastSync;
    qint64 m_clockDifference = 0;
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
