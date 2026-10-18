#pragma once
#include <QByteArray>
#include <QJsonObject>

// In-process protocol peer: uses the same handshake/validation as real devices.
class SimulatedDevice {
public:
    SimulatedDevice();
    QByteArray exchange(const QByteArray& frame);
private:
    QJsonObject m_settings;
    qint64 m_clockOffset = 0;
    qint64 m_focusDeadline = 0;
    QString m_note;
};
