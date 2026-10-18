#pragma once
#include <QObject>
#include <QBluetoothAddress>
#ifdef FLINT_BLUEZ_DBUS
#include <QDBusContext>
#include <QDBusObjectPath>
class BluezPairingAgent : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.bluez.Agent1")
public:
    explicit BluezPairingAgent(QObject* parent = nullptr) : QObject(parent) {}
    ~BluezPairingAgent() override { stop(); }
    bool start(const QBluetoothAddress& address);
    void stop();
public slots:
    Q_SCRIPTABLE void Release() { m_registered = false; }
    Q_SCRIPTABLE void Cancel() { m_peerSuffix.clear(); }
    Q_SCRIPTABLE QString RequestPinCode(const QDBusObjectPath&) { reject(); return {}; }
    Q_SCRIPTABLE uint RequestPasskey(const QDBusObjectPath&) { reject(); return 0; }
    Q_SCRIPTABLE void RequestConfirmation(const QDBusObjectPath&,uint) { reject(); }
    Q_SCRIPTABLE void DisplayPinCode(const QDBusObjectPath&,const QString&) { reject(); }
    Q_SCRIPTABLE void DisplayPasskey(const QDBusObjectPath&,uint,ushort) { reject(); }
    Q_SCRIPTABLE void RequestAuthorization(const QDBusObjectPath& device);
    Q_SCRIPTABLE void AuthorizeService(const QDBusObjectPath& device,const QString& uuid);
private:
    void reject();
    bool matches(const QDBusObjectPath& device) const;
    QString m_path, m_peerSuffix;
    bool m_registered = false;
};
#else
class BluezPairingAgent {
public:
    explicit BluezPairingAgent(QObject* = nullptr) {}
    bool start(const QBluetoothAddress&) { return true; }
    void stop() {}
};
#endif
