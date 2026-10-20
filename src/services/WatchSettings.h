#pragma once
#include "device/DeviceSession.h"
#include <QQueue>

class WatchSettings : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap values READ values NOTIFY changed)
    Q_PROPERTY(QVariantMap draft READ draft NOTIFY changed)
    Q_PROPERTY(QString state READ state NOTIFY changed)
    Q_PROPERTY(bool supported READ supported NOTIFY changed)
    Q_PROPERTY(bool dirty READ dirty NOTIFY changed)
    Q_PROPERTY(QVariantList profiles READ profiles NOTIFY profilesChanged)
    Q_PROPERTY(QVariantList faces READ faces NOTIFY facesChanged)
    Q_PROPERTY(QStringList timezones READ timezones NOTIFY timezonesChanged)
public:
    explicit WatchSettings(DeviceSession* session, QObject* parent = nullptr);
    QVariantMap values() const { return m_values; }
    QVariantMap draft() const { return m_draft; }
    QString state() const { return m_state; }
    bool supported() const;
    bool dirty() const { return m_values != m_draft; }
    QVariantList profiles() const { return m_profiles; }
    QVariantList faces() const { return m_faces; }
    QStringList timezones() const { return m_timezones; }
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void edit(const QString& key, const QVariant& value);
    Q_INVOKABLE void apply();
    Q_INVOKABLE void discard();
    Q_INVOKABLE void saveProfile(const QString& name);
    Q_INVOKABLE void loadProfile(int index);
    Q_INVOKABLE void deleteProfile(int index);
    Q_INVOKABLE bool backup(const QUrl& destination);
    Q_INVOKABLE bool restore(const QUrl& source);
    Q_INVOKABLE void selectFace(const QString& id);
signals:
    void changed();
    void profilesChanged();
    void facesChanged();
    void timezonesChanged();
private:
    void sendNext();
    void enqueue(const QString& operation, const QString& args = {});
    bool validDraft(const QVariantMap& values) const;
    void persistProfiles();
    DeviceSession* m_session;
    QVariantMap m_values, m_draft;
    QVariantList m_profiles, m_faces;
    QStringList m_timezones;
    QString m_state;
    QQueue<QPair<QString, QString>> m_queue;
    int m_request = 0;
    QString m_operation, m_deviceId;
    QVariantMap m_submittedDraft;
    bool m_loaded = false, m_wasSupported = false;
};
