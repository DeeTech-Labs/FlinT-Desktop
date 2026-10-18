#include "WatchSettings.h"
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSaveFile>
#include <QFile>
#include <QRegularExpression>
#include <cmath>

WatchSettings::WatchSettings(DeviceSession* session, QObject* parent) : QObject(parent), m_session(session) {
    const auto stored = QJsonDocument::fromJson(QSettings().value("watch/profiles").toByteArray());
    if (stored.isArray()) m_profiles = stored.array().toVariantList();
    if (m_profiles.isEmpty()) {
        m_profiles = {QVariantMap{{"name",tr("Work")},{"settings",QVariantMap{{"show_seconds",1},{"hour_format",0}}}},
                      QVariantMap{{"name",tr("Night")},{"settings",QVariantMap{{"show_seconds",0},{"hour_format",0}}}},
                      QVariantMap{{"name",tr("Minimal")},{"settings",QVariantMap{{"show_seconds",0},{"hour_format",1}}}}};
    }
    connect(session, &DeviceSession::stateChanged, this, [this] {
        if (!m_session->connected()) { m_loaded = false; m_queue.clear(); m_request = 0; m_state = tr("Disconnected"); }
        emit changed();
    });
    connect(session, &DeviceSession::sampleReceived, this, [this] {
        if (!m_loaded && supported()) { refresh(); return; }
        if (supported() && !m_request && !dirty()) {
            const auto status = m_session->status();
            auto values = status.value("settings").toMap();
            values["timezone"] = status.value("timezone");
            values["active_face"] = status.value("active_face");
            if (!values.isEmpty()) { m_values = values; m_draft = values; emit changed(); }
        }
        sendNext();
    });
    connect(session, &DeviceSession::requestFinished, this, [this](int id, bool okay, const QVariant& value, const QString& error) {
        if (id != m_request) return;
        m_request = 0;
        if (!okay) { m_state = tr("Device error: %1").arg(error); m_queue.clear(); emit changed(); return; }
        if (m_operation == "SETTINGS" || m_operation == "SET" || m_operation == "FACE") {
            m_values = value.toMap(); m_draft = m_values;
        } else if (m_operation == "FACES") m_faces = value.toList();
        else if (m_operation == "TIMEZONES") m_timezones = value.toStringList();
        m_state = tr("Applied");
        emit changed();
        sendNext();
    });
}
bool WatchSettings::supported() const {
    const auto capabilities = m_session->info().value("capabilities").toList();
    return m_session->connected() && capabilities.contains("control") && capabilities.contains("settings");
}
void WatchSettings::enqueue(const QString& operation, const QString& args) {
    m_queue.enqueue({operation,args}); sendNext();
}
void WatchSettings::sendNext() {
    if (m_request || m_queue.isEmpty() || !supported() || m_session->busy()) return;
    const auto next = m_queue.head();
    const int id = m_session->request(next.first,next.second);
    if (!id) return;
    m_queue.dequeue(); m_request = id; m_operation = next.first;
    m_state = tr("Waiting for device confirmation…"); emit changed();
}
void WatchSettings::refresh() {
    if (!supported() || m_request) return;
    m_loaded = true;
    m_queue.clear();
    m_queue.enqueue({"SETTINGS",{}}); m_queue.enqueue({"TIMEZONES",{}}); m_queue.enqueue({"FACES",{}});
    sendNext();
}
void WatchSettings::edit(const QString& key, const QVariant& value) {
    if (!m_draft.contains(key) || key == "active_face") return;
    m_draft[key] = value; m_state = tr("Pending changes"); emit changed();
}
bool WatchSettings::validDraft(const QVariantMap& values) const {
    if (values.isEmpty() || values.size() > 7) return false;
    for (auto it = values.begin(); it != values.end(); ++it) {
        if (it.key() == "timezone") {
            if (!m_timezones.contains(it.value().toString())) return false;
        } else if (it.key() == "active_face") {
            if (!QRegularExpression("^[a-zA-Z0-9_-]{1,31}$").match(it.value().toString()).hasMatch()) return false;
        } else {
            bool okay; const double raw = it.value().toDouble(&okay);
            if (!okay || !QJsonValue::fromVariant(it.value()).isDouble() || !std::isfinite(raw) || std::floor(raw) != raw || raw < 0 || raw > 255) return false;
            const int n = static_cast<int>(raw);
            if (it.key() == "show_seconds") { if (n != 0 && n != 1 && n != 255) return false; }
            else if (it.key() == "language" || it.key() == "hour_format") { if (n < 0 || n > 1) return false; }
            else if (it.key() == "date_format") { if (n < 0 || n > 5) return false; }
            else if (it.key() == "date_separator") { if (n < 0 || n > 2) return false; }
            else return false;
        }
    }
    return true;
}
void WatchSettings::apply() {
    if (!supported() || m_request || !m_queue.isEmpty()) return;
    if (!validDraft(m_draft)) { m_state = tr("Invalid settings"); emit changed(); return; }
    QStringList args;
    for (auto it = m_draft.begin(); it != m_draft.end(); ++it)
        if (it.key() != "active_face") args.append(it.key() + '=' + it.value().toString());
    m_queue.enqueue({"SET",args.join(' ')});
    if (m_draft.value("active_face") != m_values.value("active_face")) m_queue.enqueue({"FACE",m_draft.value("active_face").toString()});
    sendNext();
}
void WatchSettings::discard() { m_draft = m_values; m_state = tr("Applied"); emit changed(); }
void WatchSettings::persistProfiles() {
    QSettings().setValue("watch/profiles",QJsonDocument(QJsonArray::fromVariantList(m_profiles)).toJson(QJsonDocument::Compact));
}
void WatchSettings::saveProfile(const QString& name) {
    const auto title = name.trimmed();
    if (title.isEmpty() || title.size() > 64 || m_profiles.size() >= 50 || !validDraft(m_draft)) return;
    auto settings = m_draft; settings.remove("active_face");
    m_profiles.append(QVariantMap{{"name",title},{"settings",settings}}); persistProfiles(); emit changed();
}
void WatchSettings::loadProfile(int index) {
    if (index < 0 || index >= m_profiles.size()) return;
    const auto values = m_profiles[index].toMap().value("settings").toMap();
    auto next = m_draft;
    for (auto it = values.begin(); it != values.end(); ++it) next[it.key()] = it.value();
    if (!validDraft(next)) return;
    m_draft = next; m_state = tr("Pending changes"); emit changed();
}
void WatchSettings::deleteProfile(int index) {
    if (index < 0 || index >= m_profiles.size()) return;
    m_profiles.removeAt(index); persistProfiles(); emit changed();
}
bool WatchSettings::backup(const QUrl& destination) {
    if (!destination.isLocalFile() || m_values.isEmpty()) return false;
    const auto bytes = QJsonDocument(QJsonObject{{"schema","flint.backup/1"},{"device",QJsonObject::fromVariantMap(m_session->info())},{"settings",QJsonObject::fromVariantMap(m_values)}}).toJson();
    QSaveFile file(destination.toLocalFile());
    const bool okay = file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit();
    m_state = okay ? tr("Backup saved") : tr("Could not save backup"); emit changed(); return okay;
}
bool WatchSettings::restore(const QUrl& source) {
    QFile file(source.toLocalFile());
    if (!source.isLocalFile() || !file.open(QIODevice::ReadOnly) || file.size() > 65536) return false;
    const auto object = QJsonDocument::fromJson(file.readAll()).object();
    const auto values = object.value("settings").toObject().toVariantMap();
    if (object.value("schema").toString() != "flint.backup/1" || !validDraft(values)) {
        m_state = tr("Invalid backup"); emit changed(); return false;
    }
    m_draft = values; m_state = tr("Backup loaded. Review and apply changes."); emit changed(); return true;
}
void WatchSettings::selectFace(const QString& id) {
    if (m_request || !QRegularExpression("^[a-zA-Z0-9_-]{1,31}$").match(id).hasMatch()) return;
    enqueue("FACE",id);
}
