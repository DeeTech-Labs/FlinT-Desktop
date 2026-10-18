#pragma once
#include <QString>
#include <QBluetoothAddress>

// Own a BlueZ discovery reference even if another client is already scanning.
// Other platforms use Qt's native discovery lifecycle.
class BluezDiscoverySession {
public:
    ~BluezDiscoverySession() { stop(); }
    void start();
    void stop();
    bool disconnectPeer(const QBluetoothAddress& address);
private:
    QString m_adapterPath;
    bool m_started = false;
};
