#pragma once
#include "device/DeviceSession.h"
class Ignits : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool supported READ supported NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString state READ state NOTIFY changed)
    Q_PROPERTY(QVariantMap storage READ storage NOTIFY changed)
    Q_PROPERTY(QVariantList catalog READ catalog CONSTANT)
    Q_PROPERTY(QVariantList installed READ installed NOTIFY changed)
public:
    explicit Ignits(DeviceSession* session, QObject* parent = nullptr);
    bool supported() const;
    bool busy() const { return m_request != 0; }
    QString state() const { return m_state; }
    QVariantMap storage() const { return m_storage; }
    QVariantList catalog() const { return m_catalog; }
    Q_INVOKABLE bool canInstall(const QString& id) const;
    Q_INVOKABLE void installFromCatalog(const QString& id);
    Q_INVOKABLE void usePackage(const QString& id);
    QVariantList installed() const { return m_installed; }
    static bool encodePackage(const QByteArray& bytes, QString& wire, QString& error);
    Q_INVOKABLE void install(const QUrl& file);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void remove(const QString& id);
    Q_INVOKABLE void useTheme(const QString& id);
    Q_INVOKABLE void useFace(const QString& id);
signals:
    void changed();
    void packagesChanged();
private:
    void send(const QString& operation, const QString& arguments = {});
    DeviceSession* m_session;
    int m_request = 0;
    QString m_operation, m_state;
    QVariantList m_installed, m_catalog;
    QVariantMap m_storage;
};
