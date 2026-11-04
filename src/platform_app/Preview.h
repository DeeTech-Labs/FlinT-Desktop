#pragma once
#include "PlatformClient.h"
#include <QImage>
#include <QQuickImageProvider>
class Preview : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY changed)
    Q_PROPERTY(int revision READ revision NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
public:
    explicit Preview(PlatformClient *client);
    bool active() const {
        return pollingActive_;
    }
    void setActive(bool active);
    int revision() const {
        return revision_;
    }
    QString status() const {
        return status_;
    }
    QImage image() const {
        return image_;
    }
    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool capture(const QUrl &url);
signals:
    void changed();

private:
    PlatformClient *client_;
    QTimer timer_;
    QImage image_;
    int revision_{};
    QString status_;
    bool busy_{};
    bool pollingActive_{true};
    QString active_;
    flint::Bytes download_;
    uint32_t total_{};
    std::unique_ptr<QTemporaryDir> cache_;
    flint::Runtime runtime_;
    void scene();
    void download();
    void paint(const FlintRender::Scene &scene);
};
class PreviewProvider : public QQuickImageProvider {
    Preview *preview_;

public:
    explicit PreviewProvider(Preview *p)
        : QQuickImageProvider(QQuickImageProvider::Image), preview_(p) {}
    QImage requestImage(const QString &, QSize *size, const QSize &) override {
        auto image = preview_->image();
        if (size)
            *size = image.size();
        return image;
    }
};
