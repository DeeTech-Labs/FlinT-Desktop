#pragma once
#include "device/DeviceSession.h"
#include <QQueue>
class CompanionTools : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool actionsArmed READ actionsArmed WRITE setActionsArmed NOTIFY changed)
    Q_PROPERTY(QVariantMap actionMappings READ actionMappings NOTIFY actionMappingsChanged)
    Q_PROPERTY(bool inputEmulationAvailable READ inputEmulationAvailable NOTIFY changed)
    Q_PROPERTY(bool actionsAvailable READ actionsAvailable NOTIFY changed)
    Q_PROPERTY(QString state READ state NOTIFY changed)
    Q_PROPERTY(QString note READ note NOTIFY changed)
    Q_PROPERTY(int focusRemaining READ focusRemaining NOTIFY changed)
    Q_PROPERTY(bool supported READ supported NOTIFY changed)
    Q_PROPERTY(bool metricsEnabled READ metricsEnabled WRITE setMetricsEnabled NOTIFY changed)
    Q_PROPERTY(bool metricsAvailable READ metricsAvailable CONSTANT)
    Q_PROPERTY(int cpu READ cpu NOTIFY changed)
    Q_PROPERTY(int memory READ memory NOTIFY changed)
    Q_PROPERTY(QVariantList timeline READ timeline NOTIFY timelineChanged)
public:
    explicit CompanionTools(DeviceSession* session, QObject* parent = nullptr);
    bool actionsArmed() const { return m_actionsArmed; }
    bool actionsAvailable() const;
    bool inputEmulationAvailable() const { return actionsAvailable() && m_session->transport() != "BLE"; }
    void setActionsArmed(bool enabled);
    QVariantMap actionMappings() const { return m_actionMappings; }
    Q_INVOKABLE void setAction(const QString& button, const QString& action);
    Q_INVOKABLE void emulateButton(const QString& button);
    QString state() const { return m_state; }
    QString note() const { return m_note; }
    int focusRemaining() const;
    bool supported() const;
    bool metricsEnabled() const { return m_metricsEnabled; }
    void setMetricsEnabled(bool enabled);
    bool metricsAvailable() const;
    int cpu() const { return m_cpu; }
    int memory() const { return m_memory; }
    QVariantList timeline() const { return m_timeline; }
    Q_INVOKABLE void sendNote(const QString& note);
    Q_INVOKABLE void sendClipboard();
    Q_INVOKABLE void startFocus(int minutes);
    Q_INVOKABLE void stopFocus();
    Q_INVOKABLE void findWatch();
    Q_INVOKABLE void show(const QString& mode);
    Q_INVOKABLE bool exportTimeline(const QUrl& destination);
signals:
    void changed();
    void actionMappingsChanged();
    void timelineChanged();
private:
    void enqueue(const QString& operation, const QString& arguments = {});
    void sendNext();
    void collectMetrics();
    void dispatchAction(const QString& button);
    void record(const QString& event);
    DeviceSession* m_session;
    QString m_note, m_state, m_previousConnection;
    qint64 m_focusDeadline = 0;
    QTimer m_focusTimer, m_metricsTimer, m_actionsTimer;
    bool m_actionsArmed = false, m_cursorInitialized = false;
    quint32 m_actionCursor = 0;
    QVariantMap m_actionMappings;
    bool m_metricsEnabled = false, m_loaded = false;
    int m_cpu = -1, m_memory = -1, m_request = 0;
    quint64 m_total = 0, m_idle = 0;
    QQueue<QPair<QString,QString>> m_queue;
    QPair<QString,QString> m_active;
    QVariantList m_timeline;
};
