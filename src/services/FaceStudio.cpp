#include "FaceStudio.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QFile>
#include <QSaveFile>
#include <QRegularExpression>
#include <QFileInfo>
#include <algorithm>

FaceStudio::FaceStudio(DeviceSession* session, QObject* parent) : QObject(parent), m_session(session) {
    m_reloadTimer.setSingleShot(true); m_reloadTimer.setInterval(350);
    connect(&m_watcher,&QFileSystemWatcher::directoryChanged,this,[this] { if (m_liveReload) m_reloadTimer.start(); });
    connect(&m_watcher,&QFileSystemWatcher::fileChanged,this,[this] { if (m_liveReload) m_reloadTimer.start(); });
    connect(&m_reloadTimer,&QTimer::timeout,this,[this] {
        if (!m_liveReload || m_source.isEmpty()) return;
        QFile source(m_source);
        if (!source.open(QIODevice::ReadOnly) || source.size() > 8192) return;
        const auto next = QJsonDocument::fromJson(source.readAll()).object().toVariantMap();
        if (next == m_design) { watch(m_source); return; }
        if (importDesign(QUrl::fromLocalFile(m_source)) && supported()) { m_pendingInstall = true; install(); }
    });
    connect(session,&DeviceSession::sampleReceived,this,[this] { if (m_pendingInstall) install(); });
    m_design = {{"schema","flint.face/1"},{"name","Spark"},{"background","#101112"},{"foreground","#d4fa70"},{"timeY",128},{"dateY",180},{"showDate",true},{"pixels",QString(64,'0')}};
    const auto stored = QJsonDocument::fromJson(QSettings().value("studio/library").toByteArray()).array().toVariantList();
    for (const auto& entry : stored) if (valid(entry.toMap().value("design").toMap())) m_library.append(entry);
    if (m_library.isEmpty()) m_library.append(QVariantMap{{"design",m_design},{"favorite",true}});
    else m_design = m_library.first().toMap().value("design").toMap();
    connect(session,&DeviceSession::stateChanged,this,[this] { if (m_wasConnected == m_session->connected()) return; m_wasConnected = m_session->connected(); if (!m_session->connected()) { m_request = 0; m_pendingInstall = false; m_liveReload = false; m_reloadTimer.stop(); } emit changed(); });
    connect(session,&DeviceSession::requestFinished,this,[this](int id,bool okay,const QVariant&,const QString& error) {
        if (id != m_request) { if (m_pendingInstall) install(); return; }
        m_request = 0;
        if (!okay) m_state = tr("Device error: %1").arg(error);
        else if (!m_selecting) {
            m_selecting = true;
            m_request = m_session->request("FACE","studio");
            m_state = m_request ? tr("Selecting installed face…") : tr("Face installed. Select Studio in the gallery.");
        } else m_state = tr("Installed and selected");
        emit changed();
    });
}
bool FaceStudio::supported() const { return m_session->connected() && m_session->info().value("capabilities").toList().contains("face_studio"); }
bool FaceStudio::valid(const QVariantMap& d) {
    if ((d.size() != 8 && d.size() != 9) || d.value("schema") != "flint.face/1" || !QJsonValue::fromVariant(d.value("name")).isString() || d.value("name").toString().trimmed().isEmpty() || d.value("name").toString().size() > 64) return false;
    if (d.contains("layers")) {
        const auto layer = d.value("layers").toMap();
        if (layer.size() != 7) return false;
        for (const auto& key : {"iconX", "iconY", "iconScale", "order"}) {
            const auto v = QJsonValue::fromVariant(layer.value(key));
            if (!v.isDouble() || v.toDouble() != layer.value(key).toInt()) return false;
        }
        const int scale = layer.value("iconScale").toInt(), x = layer.value("iconX").toInt(), y = layer.value("iconY").toInt();
        if (scale < 1 || scale > 6 || x < 0 || y < 0 || x + 16*scale > 280 || y + 16*scale > 240
            || layer.value("order").toInt() < 0 || layer.value("order").toInt() > 5
            || layer.value("showIcon").metaType().id() != QMetaType::Bool
            || layer.value("animateIcon").metaType().id() != QMetaType::Bool
            || !QRegularExpression("^[0-9a-fA-F]{64}$").match(layer.value("alternatePixels").toString()).hasMatch()) return false;
    } else if (d.size() != 8) return false;
    const QRegularExpression color("^#[0-9a-fA-F]{6}$"), pixels("^[0-9a-fA-F]{64}$");
    return color.match(d.value("background").toString()).hasMatch() && color.match(d.value("foreground").toString()).hasMatch()
        && pixels.match(d.value("pixels").toString()).hasMatch()
        && QJsonValue::fromVariant(d.value("timeY")).isDouble() && d.value("timeY").toDouble() == d.value("timeY").toInt()
        && QJsonValue::fromVariant(d.value("dateY")).isDouble() && d.value("dateY").toDouble() == d.value("dateY").toInt()
        && d.value("timeY").toInt() >= 80 && d.value("timeY").toInt() <= 150
        && d.value("dateY").toInt() >= 170 && d.value("dateY").toInt() <= 220 && d.value("showDate").metaType().id() == QMetaType::Bool;
}
QVariantMap FaceStudio::layers() const {
    return m_design.contains("layers") ? m_design.value("layers").toMap() : QVariantMap{
        {"iconX",116},{"iconY",16},{"iconScale",3},{"showIcon",true},{"order",0},{"animateIcon",false},{"alternatePixels",QString(64,'0')}};
}
QString FaceStudio::framePixels() const { return m_frame == 1 ? layers().value("alternatePixels").toString() : m_design.value("pixels").toString(); }
void FaceStudio::editLayer(const QString& key, const QVariant& value) {
    auto layer = layers(); if (!layer.contains(key)) return;
    layer[key] = value;
    if (key == "iconScale") {
        layer["iconX"] = std::min(layer.value("iconX").toInt(),280 - 16*value.toInt());
        layer["iconY"] = std::min(layer.value("iconY").toInt(),240 - 16*value.toInt());
    }
    auto next = m_design; next["layers"] = layer;
    if (!valid(next)) return;
    m_design = next; m_state = tr("Unsaved design"); emit changed();
}
void FaceStudio::edit(const QString& key,const QVariant& value) {
    if (key == "schema" || !m_design.contains(key)) return;
    auto next = m_design; next[key] = value;
    if (!valid(next)) return;
    m_design = next; m_state = tr("Unsaved design"); emit changed();
}
void FaceStudio::paint(int index,bool filled) {
    if (index < 0 || index >= 256) return;
    auto pixels = QByteArray::fromHex(framePixels().toLatin1());
    const auto mask = 0x80 >> (index % 8);
    pixels[index / 8] = filled ? pixels[index / 8] | mask : pixels[index / 8] & ~mask;
    if (m_frame == 1) { editLayer("alternatePixels",QString::fromLatin1(pixels.toHex())); return; }
    m_design["pixels"] = QString::fromLatin1(pixels.toHex()); m_state = tr("Unsaved design"); emit changed();
}
void FaceStudio::persist() { emit libraryChanged(); QSettings().setValue("studio/library",QJsonDocument(QJsonArray::fromVariantList(m_library)).toJson(QJsonDocument::Compact)); }
void FaceStudio::save() {
    if (!valid(m_design)) return;
    for (auto& entry : m_library) {
        auto item = entry.toMap();
        if (item.value("design").toMap().value("name") == m_design.value("name")) {
            item["design"] = m_design; entry = item; persist(); m_state = tr("Design saved"); emit changed(); return;
        }
    }
    if (m_library.size() >= 50) return;
    m_library.append(QVariantMap{{"design",m_design},{"favorite",false}}); persist(); m_state = tr("Design saved"); emit changed();
}
void FaceStudio::load(int index) { if (index >= 0 && index < m_library.size()) { m_design = m_library[index].toMap().value("design").toMap(); emit changed(); } }
void FaceStudio::favorite(int index) {
    if (index < 0 || index >= m_library.size()) return;
    auto entry = m_library[index].toMap(); entry["favorite"] = !entry.value("favorite").toBool(); m_library[index] = entry; persist(); emit changed();
}
bool FaceStudio::importDesign(const QUrl& source) {
    QFile file(source.toLocalFile());
    if (!source.isLocalFile() || !file.open(QIODevice::ReadOnly) || file.size() > 8192) return false;
    const auto next = QJsonDocument::fromJson(file.readAll()).object().toVariantMap();
    if (!valid(next)) { m_state = tr("Invalid face package"); emit changed(); return false; }
    m_design = next; watch(file.fileName()); m_state = tr("Design imported"); emit changed(); return true;
}
bool FaceStudio::exportDesign(const QUrl& destination) {
    if (!destination.isLocalFile() || !valid(m_design)) return false;
    const auto bytes = QJsonDocument(QJsonObject::fromVariantMap(m_design)).toJson(); QSaveFile file(destination.toLocalFile());
    const bool okay = file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit();
    m_state = okay ? tr("Design exported") : tr("Could not export design"); emit changed(); return okay;
}
void FaceStudio::install() {
    if (!supported() || m_request || m_session->busy() || !valid(m_design)) return;
    auto args = QString("%1 %2 %3 %4 %5 %6").arg(m_design.value("background").toString().mid(1),m_design.value("foreground").toString().mid(1))
        .arg(m_design.value("timeY").toInt()).arg(m_design.value("dateY").toInt()).arg(m_design.value("showDate").toBool() ? 1 : 0).arg(m_design.value("pixels").toString());
    if (m_design.contains("layers")) {
        if (!m_session->info().value("capabilities").toList().contains("face_studio_layers")) {
            m_state = tr("This firmware does not support layers and animation."); emit changed(); return;
        }
        const auto layer = layers();
        args += QString(" %1 %2 %3 %4 %5 %6 %7").arg(layer.value("iconX").toInt()).arg(layer.value("iconY").toInt())
            .arg(layer.value("iconScale").toInt()).arg(layer.value("showIcon").toBool() ? 1 : 0).arg(layer.value("order").toInt())
            .arg(layer.value("animateIcon").toBool() ? 1 : 0).arg(layer.value("alternatePixels").toString());
    }
    m_selecting = false; m_request = m_session->request("STUDIO",args);
    if (m_request) m_pendingInstall = false; m_state = tr("Installing design…"); emit changed();
}

void FaceStudio::watch(const QString& file) {
    if (!m_watcher.files().isEmpty()) m_watcher.removePaths(m_watcher.files());
    if (!m_watcher.directories().isEmpty()) m_watcher.removePaths(m_watcher.directories());
    m_source = QFileInfo(file).absoluteFilePath();
    m_watcher.addPath(m_source);
    m_watcher.addPath(QFileInfo(m_source).absolutePath());
}
void FaceStudio::setLiveReload(bool enabled) {
    if (enabled && m_source.isEmpty()) { m_state = tr("Import a face file before enabling live reload."); emit changed(); return; }
    m_liveReload = enabled;
    if (!enabled) { m_reloadTimer.stop(); m_pendingInstall = false; }
    emit changed();
}
