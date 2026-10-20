#pragma once
#include "device/DeviceSession.h"
#include <QImage>
#include <QMutex>
#include <QQueue>
#include <QQuickImageProvider>
#include <memory>

struct WatchImageStore {
  QMutex mutex;
  QImage image;
};
class WatchImageProvider : public QQuickImageProvider {
public:
  explicit WatchImageProvider(std::shared_ptr<WatchImageStore> store)
      : QQuickImageProvider(Image), m_store(std::move(store)) {}
  QImage requestImage(const QString &, QSize *size, const QSize &) override;

private:
  std::shared_ptr<WatchImageStore> m_store;
};
class WatchScreen : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool active READ active WRITE setActive NOTIFY changed)
  Q_PROPERTY(bool supported READ supported NOTIFY changed)
  Q_PROPERTY(bool available READ available NOTIFY changed)
  Q_PROPERTY(bool stale READ stale NOTIFY changed)
  Q_PROPERTY(bool capturing READ capturing NOTIFY changed)
  Q_PROPERTY(QString source READ source NOTIFY imageChanged)
  Q_PROPERTY(QString state READ state NOTIFY changed)
  Q_PROPERTY(QString verification READ verification NOTIFY changed)
  Q_PROPERTY(int ageSeconds READ ageSeconds NOTIFY changed)
  Q_PROPERTY(int width READ width NOTIFY imageChanged)
  Q_PROPERTY(int height READ height NOTIFY imageChanged)
  Q_PROPERTY(quint32 revision READ revision NOTIFY imageChanged)
  Q_PROPERTY(quint64 sceneBytes READ sceneBytes NOTIFY changed)
public:
  explicit WatchScreen(DeviceSession *session, QObject *parent = nullptr);
  bool active() const { return m_active; }
  void setActive(bool value);
  bool supported() const;
  bool available() const { return !m_rgb.isEmpty() && !m_mismatch; }
  bool stale() const {
    return !m_session->connected() || !m_lastConfirmed || ageSeconds() > 6;
  }
  bool capturing() const { return m_capture || m_captureQueued; }
  QString source() const {
    return available() ? QString("image://watchscreen/%1").arg(m_imageSerial)
                       : QString();
  }
  QString state() const { return m_state; }
  QString verification() const { return m_verification; }
  int ageSeconds() const;
  int width() const { return m_width; }
  int height() const { return m_height; }
  quint32 revision() const { return m_revision; }
  quint64 sceneBytes() const { return m_sceneBytes; }
  std::shared_ptr<WatchImageStore> store() const { return m_store; }
  Q_INVOKABLE void refresh();
  Q_INVOKABLE bool sendKey(const QString &key);
  Q_INVOKABLE bool capture(const QUrl &destination);
  Q_INVOKABLE void cancelCapture();
signals:
  void changed();
  void imageChanged();
  void captureFinished(bool okay, const QString &message);

private:
  enum Stage { Idle, Metadata, Scene, ShotStart, Pixels, ShotStop, Input };
  void pump();
  void reply(bool okay, const QVariant &result, const QString &error);
  void error(const QString &text);
  bool renderScene();
  void finishPixels();
  DeviceSession *m_session;
  QTimer m_timer;
  QQueue<QString> m_inputs;
  std::shared_ptr<WatchImageStore> m_store =
      std::make_shared<WatchImageStore>();
  bool m_active = false, m_capture = false, m_mismatch = false,
       m_shotPinned = false;
  bool m_described = false, m_captureQueued = false, m_cancelRequested = false;
  quint64 m_imageSerial = 0;
  Stage m_stage = Idle;
  int m_request = 0, m_width = 0, m_height = 0, m_expectedBytes = 0;
  quint32 m_revision = 0, m_downloadingRevision = 0, m_expectedHash = 0,
          m_shotHash = 0, m_shotRevision = 0;
  quint64 m_sceneBytes = 0;
  qint64 m_lastConfirmed = 0, m_lastPoll = 0;
  QByteArray m_encoded, m_rgb, m_pixels;
  QString m_state, m_verification;
  QUrl m_destination;
};

class FaceStudio;
class FacePreviewProvider : public QQuickImageProvider {
public:
  explicit FacePreviewProvider(FaceStudio *studio)
      : QQuickImageProvider(Image), m_studio(studio) {}
  QImage requestImage(const QString &id, QSize *size, const QSize &) override;

private:
  FaceStudio *m_studio;
};
