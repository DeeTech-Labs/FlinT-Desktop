#pragma once
#include "device/DeviceSession.h"
#include <QFileSystemWatcher>
class FaceStudio : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool liveReload READ liveReload WRITE setLiveReload NOTIFY changed)
    Q_PROPERTY(QVariantMap design READ design NOTIFY changed)
    Q_PROPERTY(QVariantList library READ library NOTIFY changed)
    Q_PROPERTY(QString state READ state NOTIFY changed)
    Q_PROPERTY(bool supported READ supported NOTIFY changed)
public:
    explicit FaceStudio(DeviceSession* session, QObject* parent = nullptr);
    bool liveReload() const { return m_liveReload; }
    void setLiveReload(bool enabled);
    QVariantMap design() const { return m_design; }
    QVariantList library() const { return m_library; }
    QString state() const { return m_state; }
    bool supported() const;
    Q_INVOKABLE void edit(const QString& key, const QVariant& value);
    Q_INVOKABLE void paint(int index, bool filled);
    Q_INVOKABLE void save();
    Q_INVOKABLE void load(int index);
    Q_INVOKABLE void favorite(int index);
    Q_INVOKABLE bool importDesign(const QUrl& source);
    Q_INVOKABLE bool exportDesign(const QUrl& destination);
    Q_INVOKABLE void install();
signals:
    void changed();
private:
    static bool valid(const QVariantMap& design);
    void persist();
    void watch(const QString& file);
    QFileSystemWatcher m_watcher;
    QTimer m_reloadTimer;
    QString m_source;
    bool m_liveReload = false;
    bool m_pendingInstall = false;
    DeviceSession* m_session;
    QVariantMap m_design;
    QVariantList m_library;
    QString m_state;
    int m_request = 0;
    bool m_selecting = false;
};
