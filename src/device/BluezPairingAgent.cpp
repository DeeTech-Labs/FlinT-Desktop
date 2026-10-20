#include "BluezPairingAgent.h"
#ifdef FLINT_BLUEZ_DBUS
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

bool BluezPairingAgent::start(const QBluetoothAddress& address) {
    stop();
    auto peer = address.toString().toUpper(); peer.replace(':','_');
    m_peerSuffix = "/dev_"+peer;
    m_path = "/org/deetech/flint/agent_"+QString::number(quintptr(this),16);
    auto bus = QDBusConnection::systemBus();
    if (!bus.registerObject(m_path,this,QDBusConnection::ExportScriptableSlots)) { m_path.clear(); return false; }
    QDBusInterface manager("org.bluez","/org/bluez","org.bluez.AgentManager1",bus);
    manager.setTimeout(3000);
    const QDBusReply<void> reply = manager.call("RegisterAgent",QVariant::fromValue(QDBusObjectPath(m_path)),"DisplayYesNo");
    m_registered = reply.isValid();
    if (!m_registered) { bus.unregisterObject(m_path); m_path.clear(); }
    return m_registered;
}
void BluezPairingAgent::stop() {
    confirm(false);
    // Revoke authorization before any nested D-Bus calls can process requests.
    m_peerSuffix.clear();
    if (m_path.isEmpty()) return;
    auto bus = QDBusConnection::systemBus();
    if (m_registered) {
        m_registered = false;
        QDBusInterface manager("org.bluez","/org/bluez","org.bluez.AgentManager1",bus);
        manager.setTimeout(3000);
        manager.call("UnregisterAgent",QVariant::fromValue(QDBusObjectPath(m_path)));
    }
    bus.unregisterObject(m_path); m_path.clear();
}
bool BluezPairingAgent::matches(const QDBusObjectPath& device) const {
    return m_registered && !m_peerSuffix.isEmpty() && device.path().endsWith(m_peerSuffix);
}
void BluezPairingAgent::RequestConfirmation(const QDBusObjectPath& device, uint passkey) {
    if (!matches(device) || !calledFromDBus() || !m_confirmation.isEmpty()) { reject(); return; }
    setDelayedReply(true);
    m_pending = message();
    m_confirmation = QString("%1").arg(passkey, 6, 10, QLatin1Char('0'));
    m_confirmationTimeout.setSingleShot(true);
    m_confirmationTimeout.setInterval(25000);
    disconnect(&m_confirmationTimeout, nullptr, this, nullptr);
    connect(&m_confirmationTimeout, &QTimer::timeout, this, [this] { confirm(false); }, Qt::SingleShotConnection);
    m_confirmationTimeout.start();
    emit confirmationChanged();
}
void BluezPairingAgent::confirm(bool accepted) {
    if (m_confirmation.isEmpty()) return;
    m_confirmationTimeout.stop();
    QDBusConnection::systemBus().send(accepted ? m_pending.createReply()
        : m_pending.createErrorReply("org.bluez.Error.Rejected", "Pairing was declined or timed out"));
    m_pending = QDBusMessage();
    m_confirmation.clear();
    emit confirmationChanged();
}
void BluezPairingAgent::reject() {
    if (calledFromDBus()) sendErrorReply("org.bluez.Error.Rejected","Pairing requires user input or targets an unexpected device");
}
void BluezPairingAgent::RequestAuthorization(const QDBusObjectPath& device) {
    // Explicit Connect authorizes only the selected peer; numeric confirmation stays explicit.
    if (!matches(device)) reject();
}
void BluezPairingAgent::AuthorizeService(const QDBusObjectPath& device,const QString& uuid) {
    const bool allowed = uuid == "e7f10001-7a1e-4b5c-9d2f-46544c494e54"
        || uuid == "00001800-0000-1000-8000-00805f9b34fb" || uuid == "00001801-0000-1000-8000-00805f9b34fb";
    if (!matches(device) || !allowed) reject();
}
#endif
