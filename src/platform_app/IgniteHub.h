#pragma once
#include "PlatformClient.h"
#include <QUrl>
class IgniteHub : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList packages READ packages NOTIFY changed)
    Q_PROPERTY(QVariantList permissions READ permissions NOTIFY changed)
    Q_PROPERTY(QString selected READ selected NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(double progress READ progress NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
public:
    explicit IgniteHub(PlatformClient *client);
    QVariantList packages() const {
        return packages_;
    }
    QVariantList permissions() const {
        return permissions_;
    }
    QString selected() const {
        return selected_;
    }
    bool busy() const {
        return busy_;
    }
    double progress() const {
        return progress_;
    }
    QString message() const {
        return message_;
    }
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void select(const QString &id);
    Q_INVOKABLE void install(const QUrl &path);
    Q_INVOKABLE void launch(const QString &id, const QString &component);
    Q_INVOKABLE void remove(const QString &id);
    Q_INVOKABLE void rollback(const QString &id);
    Q_INVOKABLE void setRole(int role, const QString &id, const QString &component);
    Q_INVOKABLE void grant(const QString &name, const QString &scope, bool enabled);
signals:
    void changed();
    void packagesChanged();

private:
    PlatformClient *client_;
    QVariantList packages_, permissions_, incoming_;
    QString selected_, message_;
    bool busy_{};
    double progress_{};
    QByteArray install_;
    uint32_t offset_{};
    uint64_t catalogRequest_{};
    void page(uint32_t offset, uint64_t generation);
    void chunk();
    void result(flint::Error error);
    void permissionsRefresh();
};
