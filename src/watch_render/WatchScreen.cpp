#include "WatchScreen.h"
#include "services/FaceStudio.h"
#include "shared/builders.h"
#include "shared/version.h"
#include <QDateTime>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QSaveFile>

QImage WatchImageProvider::requestImage(const QString &, QSize *size,
                                        const QSize &) {
  QMutexLocker lock(&m_store->mutex);
  if (size)
    *size = m_store->image.size();
  return m_store->image;
}
namespace {
QImage imageFrom565(const QByteArray &rgb, int width, int height) {
  QImage image(width, height, QImage::Format_RGB16);
  for (int y = 0; y < height; ++y) {
    auto *row = reinterpret_cast<quint16 *>(image.scanLine(y));
    for (int x = 0; x < width; ++x) {
      const auto *p = reinterpret_cast<const uint8_t *>(rgb.constData()) +
                      (y * width + x) * 2;
      row[x] = FlintRender::get16(p);
    }
  }
  return image;
}
bool unsignedValue(const QVariant &v, quint32 &out) {
  bool ok = false;
  const auto n = v.toULongLong(&ok);
  if (!ok || n > UINT32_MAX)
    return false;
  out = quint32(n);
  return true;
}
} // namespace
WatchScreen::WatchScreen(DeviceSession *session, QObject *parent)
    : QObject(parent), m_session(session) {
  m_timer.setInterval(200);
  connect(&m_timer, &QTimer::timeout, this, &WatchScreen::pump);
  m_timer.start();
  connect(session, &DeviceSession::stateChanged, this, [this] {
    if (!m_session->connected()) {
      if (capturing()) {
        m_capture = false;
        m_verification = tr("Capture interrupted by disconnection");
        emit captureFinished(false, m_verification);
      }
      m_request = 0;
      m_inputs.clear();
      m_stage = Idle;
      m_shotPinned = false;
      m_lastPoll = 0;
      m_lastConfirmed = 0;
      m_described = false;
      m_captureQueued = false;
      m_cancelRequested = false;
    }
    emit changed();
  });
  connect(
      session, &DeviceSession::requestFinished, this,
      [this](int id, bool okay, const QVariant &result, const QString &text) {
        if (id != m_request)
          return;
        m_request = 0;
        reply(okay, result, text);
      });
}
bool WatchScreen::supported() const {
  return m_session->connected() && m_session->info()
                                       .value("capabilities")
                                       .toList()
                                       .contains("screen_scene");
}
int WatchScreen::ageSeconds() const {
  return m_lastConfirmed
             ? int((QDateTime::currentMSecsSinceEpoch() - m_lastConfirmed) /
                   1000)
             : 0;
}
void WatchScreen::setActive(bool value) {
  if (value == m_active)
    return;
  m_active = value;
  if (value) {
    m_lastPoll = 0;
  }
  emit changed();
  pump();
}
void WatchScreen::refresh() {
  m_lastPoll = 0;
  pump();
}
bool WatchScreen::sendKey(const QString &key) {
  if (!supported() || m_session->transport() == "BLE" ||
      m_inputs.size() >= 16 ||
      !QStringList{"previous", "next", "enter", "back", "settings", "plus",
                   "minus"}
           .contains(key))
    return false;
  m_inputs.enqueue(key);
  pump();
  return true;
}
void WatchScreen::pump() {
  if (!supported() || m_request || m_session->busy())
    return;
  const auto now = QDateTime::currentMSecsSinceEpoch();
  if (m_stage == Idle && !m_inputs.isEmpty())
    m_stage = Input;
  if (m_stage == Idle && m_captureQueued) {
    m_captureQueued = false;
    m_capture = true;
    m_stage = ShotStart;
  }
  if (m_stage == Idle) {
    if (!m_active && !m_capture)
      return;
    const int interval = m_session->transport() == "BLE" ? 2500 : 1000;
    if (m_lastPoll && now - m_lastPoll < interval)
      return;
    m_stage = Metadata;
  }
  QString operation, args;
  switch (m_stage) {
  case Input:
    operation = "KEY";
    args = m_inputs.head();
    break;
  case Metadata:
    operation = "SCREEN";
    if (m_described && !m_capture && !m_mismatch && m_revision)
      args = QString::number(m_revision);
    break;
  case Scene:
    operation = "SCENE";
    args = QString::number(m_downloadingRevision) + ' ' +
           QString::number(m_encoded.size());
    break;
  case ShotStart:
    operation = "SHOT";
    args = "start";
    break;
  case Pixels:
    operation = "PIXELS";
    args = QString::number(m_shotRevision) + ' ' +
           QString::number(m_pixels.size());
    break;
  case ShotStop:
    operation = "SHOT";
    args = "stop";
    break;
  default:
    return;
  }
  m_request = m_session->request(operation, args);
  if (m_request && m_stage == Input)
    m_inputs.dequeue();
  if (m_request && m_stage == Metadata)
    m_lastPoll = now;
}
void WatchScreen::error(const QString &text) {
  m_state = text;
  m_lastConfirmed = 0;
  m_encoded.clear();
  m_pixels.clear();
  if (m_capture) {
    m_verification = text;
    emit captureFinished(false, text);
  }
  m_capture = false;
  m_captureQueued = false;
  m_cancelRequested = false;
  m_stage = m_shotPinned ? ShotStop : Idle;
  emit changed();
}
void WatchScreen::reply(bool okay, const QVariant &value, const QString &text) {
  if (!okay) {
    if (m_stage == ShotStop) {
      m_shotPinned = false;
      m_stage = Idle;
      emit changed();
      return;
    }
    error(tr("Screen unavailable: %1").arg(text));
    return;
  }
  const auto result = value.toMap();
  if (m_stage == Input) {
    m_stage = Idle;
    m_lastPoll = 0;
    pump();
    return;
  }
  if (m_cancelRequested && m_stage != ShotStop) {
    if (m_stage == ShotStart)
      m_shotPinned = true;
    m_cancelRequested = false;
    m_capture = false;
    m_captureQueued = false;
    m_stage = m_shotPinned ? ShotStop : Idle;
    emit changed();
    pump();
    return;
  }
  quint32 revision = 0;
  if (m_stage == ShotStop) {
    m_shotPinned = false;
    m_capture = false;
    m_stage = Idle;
    emit changed();
    return;
  }
  if (m_stage == ShotStart) {
    m_shotPinned = true;
    if (!unsignedValue(result.value("revision"), m_shotRevision) ||
        !unsignedValue(result.value("hash"), m_shotHash) ||
        result.value("pixels").toInt() < 1 ||
        result.value("pixels").toInt() > 320 * 320) {
      error(tr("Invalid screen capture metadata"));
      return;
    }
    m_shotPinned = true;
    m_stage = Metadata;
    pump();
    return;
  }
  if (m_stage == Metadata) {
    if (!unsignedValue(result.value("revision"), revision)) {
      error(tr("Invalid screen metadata"));
      return;
    }
    if (result.value("unchanged").toBool() && revision == m_revision &&
        !m_capture) {
      m_lastConfirmed = QDateTime::currentMSecsSinceEpoch();
      m_stage = Idle;
      emit changed();
      return;
    }
    if (!result.value("available").toBool()) {
      error(tr("This screen cannot be reproduced by this firmware"));
      return;
    }
    if (result.value("renderer").toString() != FlintRender::RendererId) {
      m_mismatch = true;
      error(
          tr("Visual resources differ. Update Desktop and firmware together."));
      emit imageChanged();
      return;
    }
    const int width = result.value("width").toInt(),
              height = result.value("height").toInt(),
              bytes = result.value("bytes").toInt();
    if (width < 1 || width > 320 || height < 1 || height > 320 ||
        bytes < FlintRender::HeaderSize ||
        bytes > int(FlintRender::MaxEncoded) ||
        !unsignedValue(result.value("hash"), m_expectedHash) ||
        (m_capture && revision != m_shotRevision)) {
      error(tr("Invalid screen metadata"));
      return;
    }
    m_width = width;
    m_height = height;
    m_expectedBytes = bytes;
    m_downloadingRevision = revision;
    m_encoded.clear();
    m_stage = Scene;
    pump();
    return;
  }
  if (m_stage == Scene || m_stage == Pixels) {
    if (!unsignedValue(result.value("revision"), revision)) {
      error(tr("Invalid screen block"));
      return;
    }
    auto &target = m_stage == Scene ? m_encoded : m_pixels;
    const auto hex = result.value("data").toString();
    if (revision !=
            (m_stage == Scene ? m_downloadingRevision : m_shotRevision) ||
        result.value("offset").toInt() != target.size() || hex.isEmpty() ||
        hex.size() > 448 || hex.size() % 2 ||
        !QRegularExpression("^[0-9a-f]+$").match(hex).hasMatch()) {
      error(tr("Invalid screen block"));
      return;
    }
    const auto bytes = QByteArray::fromHex(hex.toLatin1());
    const int expected =
        m_stage == Scene ? m_expectedBytes : m_width * m_height * 2;
    if (target.size() + bytes.size() > expected) {
      error(tr("Invalid screen block"));
      return;
    }
    target += bytes;
    if (target.size() == expected) {
      if (m_stage == Scene) {
        if (!renderScene())
          return;
        m_stage = m_capture ? Pixels : Idle;
        m_pixels.clear();
      } else {
        finishPixels();
        return;
      }
    }
    pump();
  }
}
bool WatchScreen::renderScene() {
  FlintRender::Scene scene;
  if (FlintRender::hash(
          reinterpret_cast<const uint8_t *>(m_encoded.constData()),
          m_encoded.size()) != m_expectedHash ||
      !FlintRender::decode(
          reinterpret_cast<const uint8_t *>(m_encoded.constData()),
          m_encoded.size(), scene) ||
      scene.width != m_width || scene.height != m_height) {
    error(tr("Screen integrity check failed"));
    return false;
  }
  QVector<uint16_t> pixels(m_width * m_height);
  FlintRender::Canvas canvas(pixels.data(), m_width, m_height);
  FlintRender::render(scene, canvas);
  m_rgb.resize(m_width * m_height * 2);
  for (int i = 0; i < pixels.size(); ++i)
    FlintRender::put16(reinterpret_cast<uint8_t *>(m_rgb.data()) + i * 2,
                       pixels[i]);
  {
    QMutexLocker lock(&m_store->mutex);
    m_store->image = imageFrom565(m_rgb, m_width, m_height);
  }
  m_revision = m_downloadingRevision;
  ++m_imageSerial;
  m_described = true;
  m_mismatch = false;
  m_lastConfirmed = QDateTime::currentMSecsSinceEpoch();
  m_sceneBytes += m_encoded.size();
  m_state = tr("Confirmed device screen");
  emit imageChanged();
  emit changed();
  return true;
}
bool WatchScreen::capture(const QUrl &destination) {
  if (!supported() || capturing() || m_session->transport() == "BLE" ||
      !destination.isLocalFile())
    return false;
  m_destination = destination;
  m_captureQueued = true;
  m_verification =
      tr("Reading actual pixels. Watch display is temporarily held.");
  m_pixels.clear();
  emit changed();
  pump();
  return true;
}
void WatchScreen::cancelCapture() {
  if (!capturing())
    return;
  m_captureQueued = false;
  m_verification = tr("Capture cancelled");
  if (m_request && m_capture) {
    m_cancelRequested = true;
  } else if (m_capture) {
    m_capture = false;
    m_stage = m_shotPinned ? ShotStop : Idle;
  }
  emit changed();
  pump();
}
void WatchScreen::finishPixels() {
  const auto hash = FlintRender::hash(
      reinterpret_cast<const uint8_t *>(m_pixels.constData()), m_pixels.size());
  const bool matched = hash == m_shotHash && m_pixels == m_rgb;
  bool saved = false;
  if (hash == m_shotHash) {
    QSaveFile file(m_destination.toLocalFile());
    if (file.open(QIODevice::WriteOnly))
      saved = imageFrom565(m_pixels, m_width, m_height).save(&file, "PNG") &&
              file.commit();
  }
  m_verification =
      matched
          ? (saved ? tr("Pixel comparison passed. Screenshot saved.")
                   : tr("Pixels match, but the screenshot could not be saved."))
          : tr("Pixel mismatch: preview has been disabled.");
  if (!matched) {
    m_mismatch = true;
    emit imageChanged();
  }
  m_state = m_verification;
  m_pixels.clear();
  m_capture = false;
  m_stage = ShotStop;
  emit changed();
  emit captureFinished(matched && saved, m_verification);
  pump();
}

QImage FacePreviewProvider::requestImage(const QString &id, QSize *size,
                                         const QSize &) {
  const auto design = m_studio->design(), layers = m_studio->layers();
  const auto bits = QByteArray::fromHex(
      (layers.value("animateIcon").toBool() && id.endsWith("/1")
           ? layers.value("alternatePixels")
           : design.value("pixels"))
          .toString()
          .toLatin1());
  if (bits.size() != 32)
    return {};
  const auto scene = FlintRender::studio(
      design.value("background").toString().mid(1).toUInt(nullptr, 16),
      design.value("foreground").toString().mid(1).toUInt(nullptr, 16),
      reinterpret_cast<const uint8_t *>(bits.constData()),
      layers.value("iconX").toInt(), layers.value("iconY").toInt(),
      layers.value("iconScale").toInt(), layers.value("showIcon").toBool(),
      layers.value("order").toUInt(), "12:34", FlintRender::TimeLarge,
      design.value("timeY").toInt(),
      design.value("showDate").toBool() ? "20.10.2026" : "",
      design.value("dateY").toInt());
  QImage image(280, 240, QImage::Format_RGB16);
  FlintRender::Canvas canvas(reinterpret_cast<uint16_t *>(image.bits()), 280,
                             240);
  FlintRender::render(scene, canvas);
  if (size)
    *size = image.size();
  return image;
}
