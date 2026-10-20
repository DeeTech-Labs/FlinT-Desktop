#pragma once
#include <QByteArray>
#include <QJsonObject>
#include <QJsonArray>

// In-process protocol peer: uses the same handshake/validation as real devices.
class SimulatedDevice {
public:
    SimulatedDevice();
    QByteArray exchange(const QByteArray& frame);
private:
    void updateScreen();
    QJsonValue screenCommand(const QString& operation, const QString& args, QString& error);
    QByteArray m_scene, m_transferScene, m_screenPixels;
    quint32 m_screenRevision = 0, m_transferRevision = 0;
    qint64 m_pinUntil = 0;
    int m_menu = 0, m_selection = 0;
    quint32 m_actionSequence = 0;
    QJsonArray m_actionEvents;
    QJsonObject m_settings;
    qint64 m_clockOffset = 0;
    qint64 m_focusDeadline = 0;
    QString m_note;
};
