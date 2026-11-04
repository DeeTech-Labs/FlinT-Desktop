#pragma once
#include "PlatformClient.h"
class CompanionService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool metricsAllowed READ metricsAllowed WRITE setMetricsAllowed NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
public:
    explicit CompanionService(PlatformClient *client);
    bool metricsAllowed() const {
        return allowed_;
    }
    void setMetricsAllowed(bool value);
    QString status() const {
        return status_;
    }
signals:
    void changed();

private:
    PlatformClient *client_;
    QTimer timer_;
    bool allowed_{};
    QString status_{"Передача CPU/RAM выключена"};
    quint64 total_{}, idle_{};
    void sample();
};
