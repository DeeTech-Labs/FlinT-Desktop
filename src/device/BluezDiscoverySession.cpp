#include "BluezDiscoverySession.h"
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#include <QBluetoothLocalDevice>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QDebug>

namespace {
using Interfaces = QMap<QString,QVariantMap>;
using Objects = QMap<QDBusObjectPath,Interfaces>;
}
void BluezDiscoverySession::start() {
    if (m_started) return;
    qDBusRegisterMetaType<Interfaces>();
    qDBusRegisterMetaType<Objects>();
    const auto address = QBluetoothLocalDevice().address().toString();
    QDBusInterface manager("org.bluez","/","org.freedesktop.DBus.ObjectManager",QDBusConnection::systemBus());
    manager.setTimeout(3000);
    const QDBusReply<Objects> objects = manager.call("GetManagedObjects");
    if (!objects.isValid()) return; // Qt will report adapter/service errors.
    const auto managed = objects.value();
    for (auto it = managed.cbegin(); it != managed.cend(); ++it) {
        const auto properties = it.value().value("org.bluez.Adapter1");
        if (properties.value("Address").toString().compare(address,Qt::CaseInsensitive) != 0) continue;
        QDBusInterface adapter("org.bluez",it.key().path(),"org.bluez.Adapter1",QDBusConnection::systemBus());
        adapter.setTimeout(3000);
        const QVariantMap filter{{"Transport","le"}};
        const QDBusReply<void> filtered = adapter.call("SetDiscoveryFilter",filter);
        if (!filtered.isValid()) return;
        const QDBusReply<void> started = adapter.call("StartDiscovery");
        if (started.isValid()) { m_adapterPath = it.key().path(); m_started = true; }
        else qWarning() << "BlueZ discovery session:" << started.error().message();
        return;
    }
}
void BluezDiscoverySession::stop() {
    if (!m_started) return;
    m_started = false;
    QDBusInterface adapter("org.bluez",m_adapterPath,"org.bluez.Adapter1",QDBusConnection::systemBus());
    adapter.setTimeout(3000);
    adapter.call("StopDiscovery"); // Releases this process's reference only.
}
bool BluezDiscoverySession::disconnectPeer(const QBluetoothAddress& address) {
    if (m_adapterPath.isEmpty() || address.isNull()) return false;
    auto peer = address.toString(); peer.replace(':','_');
    QDBusInterface device("org.bluez",m_adapterPath+"/dev_"+peer,"org.bluez.Device1",QDBusConnection::systemBus());
    device.setTimeout(3000);
    const QDBusReply<void> reply = device.call("Disconnect");
    return reply.isValid() || reply.error().name() == "org.bluez.Error.NotConnected";
}
#else
void BluezDiscoverySession::start() {}
void BluezDiscoverySession::stop() {}
bool BluezDiscoverySession::disconnectPeer(const QBluetoothAddress&) { return false; }
#endif
