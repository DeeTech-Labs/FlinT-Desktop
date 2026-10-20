#include "CompanionTools.h"
#include <QDateTime>
#include <QFile>
#include <QSaveFile>
#include <QSettings>
#include <QJsonArray>
#include <QJsonDocument>
#include <QClipboard>
#include <QGuiApplication>
#include <QRegularExpression>
#include <algorithm>
#include <QProcess>
#include <QStandardPaths>
#ifdef FLINT_BLUEZ_DBUS
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#endif

CompanionTools::CompanionTools(DeviceSession* session,QObject* parent) : QObject(parent),m_session(session) {
    m_actionMappings = QSettings().value("actions/mappings", QVariantMap{{"primary","none"},{"secondary","none"},{"long","none"}}).toMap();
    m_actionsTimer.setInterval(750);
    connect(&m_actionsTimer, &QTimer::timeout, this, [this] {
        if (!actionsAvailable()) return;
        for (const auto& entry : m_queue) if (entry.first == "EVENTS") return;
        if (m_request && m_active.first == "EVENTS") return;
        enqueue("EVENTS", m_cursorInitialized ? QString::number(m_actionCursor) : QString());
    });
    m_actionsTimer.start();
    m_timeline = QJsonDocument::fromJson(QSettings().value("companion/timeline").toByteArray()).array().toVariantList();
    while (m_timeline.size() > 200) m_timeline.removeFirst();
    m_focusTimer.setInterval(1000);
    connect(&m_focusTimer,&QTimer::timeout,this,[this] { if (!focusRemaining()) m_focusTimer.stop(); emit changed(); });
    m_metricsTimer.setInterval(3000);
    connect(&m_metricsTimer,&QTimer::timeout,this,&CompanionTools::collectMetrics);
    connect(session,&DeviceSession::stateChanged,this,[this] {
        const auto connection = m_session->connectionState();
        const auto key = m_session->transport() + (m_session->connected() ? "connected" : m_session->connecting() ? "connecting" : m_session->autoConnect() ? "searching" : "disconnected");
        if (key != m_previousConnection) { m_previousConnection = key; record(connection); }
        if (!m_session->connected()) { m_actionsArmed = false; m_cursorInitialized = false; m_actionCursor = 0; m_loaded = false; m_queue.clear(); m_request = 0; }
        emit changed();
    });
    connect(session,&DeviceSession::sampleReceived,this,[this] { if (supported() && !m_loaded) { m_loaded = true; enqueue("APPS"); } else sendNext(); });
    connect(session,&DeviceSession::requestFinished,this,[this](int id,bool okay,const QVariant& result,const QString& error) {
        if (id != m_request) return;
        m_request = 0;
        if (!okay) { m_state = tr("Device error: %1").arg(error); m_queue.clear(); emit changed(); return; }
        if (m_active.first == "EVENTS") {
            const auto data = result.toMap();
            const auto sequence = data.value("sequence").toUInt();
            if (m_cursorInitialized && sequence >= m_actionCursor && !data.value("lost").toBool()) {
                for (const auto& event : data.value("events").toList()) {
                    const auto entry = event.toMap();
                    const auto id = entry.value("id").toUInt();
                    if (id > m_actionCursor && id <= sequence) {
                        m_actionCursor = id;
                        if (m_actionsArmed) dispatchAction(entry.value("button").toString());
                    }
                }
            } else if (data.value("lost").toBool()) record(tr("Input events lost; skipped for safety"));
            m_actionCursor = sequence; m_cursorInitialized = true;
            emit changed(); sendNext(); return;
        }
        if (m_active.first == "APPS") {
            const auto data = result.toMap(); m_note = data.value("note").toString();
            m_focusDeadline = QDateTime::currentMSecsSinceEpoch()+data.value("focus_remaining").toInt()*1000LL;
            if (focusRemaining()) m_focusTimer.start();
        } else if (m_active.first == "NOTE") m_note = QString::fromUtf8(QByteArray::fromHex(m_active.second.toLatin1()));
        else if (m_active.first == "FOCUS") { m_focusDeadline = QDateTime::currentMSecsSinceEpoch()+m_active.second.toInt()*1000LL; m_focusTimer.start(); }
        m_state = tr("Confirmed by watch");
        if (m_active.first != "PC" && m_active.first != "APPS") record(m_active.first);
        emit changed(); sendNext();
    });
}
bool CompanionTools::supported() const { return m_session->connected() && m_session->info().value("capabilities").toList().contains("companion"); }
bool CompanionTools::actionsAvailable() const {
    return supported() && m_session->info().value("capabilities").toList().contains("actions");
}
void CompanionTools::setActionsArmed(bool enabled) {
    m_actionsArmed = enabled && actionsAvailable();
    m_cursorInitialized = false; // Discard events received before explicit arming.
    record(m_actionsArmed ? tr("Actions enabled for this session") : tr("Actions disabled"));
    emit changed();
}
void CompanionTools::setAction(const QString& button, const QString& action) {
    if (!QStringList{"primary","secondary","long"}.contains(button)
        || !QStringList{"none","play_pause","next","previous","focus","focus_stop","presentation_next","presentation_previous"}.contains(action)) return;
    m_actionMappings.insert(button, action);
    QSettings().setValue("actions/mappings", m_actionMappings);
    emit actionMappingsChanged();
    emit changed();
}
void CompanionTools::emulateButton(const QString& button) {
    if (!actionsAvailable() || m_session->transport() == "BLE") return;
    if (QStringList{"primary","secondary","long"}.contains(button)) enqueue("BUTTON", button);
}
void CompanionTools::dispatchAction(const QString& button) {
    const auto action = m_actionMappings.value(button, "none").toString();
    if (action == "none") return;
    if (action == "focus") startFocus(25);
    else if (action == "focus_stop") stopFocus();
    else if (action.startsWith("presentation_")) {
        const auto tool = QStandardPaths::findExecutable("xdotool");
        if (qEnvironmentVariable("XDG_SESSION_TYPE") != "x11" || tool.isEmpty()) {
            record(tr("Presentation keys require X11 and xdotool")); return;
        }
        if (!QProcess::startDetached(tool, {"key", "--clearmodifiers", action == "presentation_next" ? "Right" : "Left"})) {
            record(tr("Action failed")); return;
        }
    } else {
#ifdef FLINT_BLUEZ_DBUS
        const auto bus = QDBusConnection::sessionBus();
        const auto names = bus.interface()->registeredServiceNames();
        QStringList players;
        if (names.isValid()) for (const auto& name : names.value()) if (name.startsWith("org.mpris.MediaPlayer2.")) players.append(name);
        players.sort();
        if (players.isEmpty()) { record(tr("No media player available")); return; }
        QString selected = players.first();
        for (const auto& player : players) {
            QDBusInterface candidate(player, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", bus);
            if (candidate.property("PlaybackStatus").toString() == "Playing") { selected = player; break; }
        }
        QDBusInterface player(selected, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", bus);
        const auto method = action == "play_pause" ? "PlayPause" : action == "next" ? "Next" : "Previous";
        auto* watcher = new QDBusPendingCallWatcher(player.asyncCall(method), this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher, action] {
            QDBusPendingReply<> reply = *watcher;
            record(reply.isError() ? tr("Action failed") : action);
            watcher->deleteLater();
        });
        return;
#else
        record(tr("Media actions are currently available on Linux")); return;
#endif
    }
    record(action);
}
int CompanionTools::focusRemaining() const { return std::max<qint64>(0,(m_focusDeadline-QDateTime::currentMSecsSinceEpoch()+999)/1000); }
bool CompanionTools::metricsAvailable() const {
#ifdef Q_OS_LINUX
    return QFile::exists("/proc/stat") && QFile::exists("/proc/meminfo");
#else
    return false;
#endif
}
void CompanionTools::record(const QString& event) {
    m_timeline.append(QVariantMap{{"at",QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},{"event",event}});
    while (m_timeline.size() > 200) m_timeline.removeFirst();
    QSettings().setValue("companion/timeline",QJsonDocument(QJsonArray::fromVariantList(m_timeline)).toJson(QJsonDocument::Compact));
    emit timelineChanged();
    emit changed();
}
void CompanionTools::enqueue(const QString& operation,const QString& arguments) {
    if (!supported() || m_queue.size() >= 20) return;
    if (operation == "PC") for (const auto& command : m_queue) if (command.first == "PC") return;
    m_queue.enqueue({operation,arguments}); sendNext();
}
void CompanionTools::sendNext() {
    if (!supported() || m_request || m_queue.isEmpty() || m_session->busy()) return;
    m_active = m_queue.head(); const auto id = m_session->request(m_active.first,m_active.second);
    if (!id) return;
    m_queue.dequeue(); m_request = id; m_state = tr("Waiting for watch…"); emit changed();
}
void CompanionTools::sendNote(const QString& note) {
    const auto text = note.simplified(); const auto bytes = text.toUtf8();
    if (bytes.size() > 96) { m_state = tr("The note must fit in 96 UTF-8 bytes."); emit changed(); return; }
    enqueue("NOTE",QString::fromLatin1(bytes.toHex())); enqueue("FACE","companion");
}
void CompanionTools::sendClipboard() { sendNote(QGuiApplication::clipboard()->text()); }
void CompanionTools::startFocus(int minutes) { if (minutes < 1 || minutes > 1440) return; enqueue("FOCUS",QString::number(minutes*60)); enqueue("FACE","companion"); }
void CompanionTools::stopFocus() { enqueue("FOCUS","0"); }
void CompanionTools::findWatch() { enqueue("FIND"); }
void CompanionTools::show(const QString& mode) { if (mode != "note" && mode != "focus" && mode != "computer") return; enqueue("SHOW",mode); enqueue("FACE","companion"); }
void CompanionTools::setMetricsEnabled(bool enabled) {
    m_metricsEnabled = enabled && metricsAvailable();
    if (m_metricsEnabled) { collectMetrics(); m_metricsTimer.start(); show("computer"); }
    else {
        m_metricsTimer.stop();
        for (auto it = m_queue.begin(); it != m_queue.end();) { if (it->first == "PC") it = m_queue.erase(it); else ++it; }
    }
    emit changed();
}
void CompanionTools::collectMetrics() {
#ifdef Q_OS_LINUX
    QFile stat("/proc/stat"), memory("/proc/meminfo");
    if (!stat.open(QIODevice::ReadOnly) || !memory.open(QIODevice::ReadOnly)) return;
    const auto fields = stat.readLine().simplified().split(' ');
    if (fields.size() >= 9) {
        quint64 total = 0;
        // guest/guest_nice are already included in user/nice; count only 8 fields.
        for (int index = 1; index <= 8; ++index) total += fields[index].toULongLong();
        const quint64 idle = fields[4].toULongLong()+fields[5].toULongLong();
        if (total > m_total && m_total && idle >= m_idle) m_cpu = std::clamp<int>(qRound(100.0*(1.0-double(idle-m_idle)/double(total-m_total))),0,100);
        m_total = total; m_idle = idle;
    }
    quint64 totalMemory = 0, available = 0;
    const auto lines = memory.readAll().split('\n');
    for (const auto& line : lines) {
        const auto parts = line.simplified().split(' ');
        if (parts.size() < 2) continue;
        if (parts[0] == "MemTotal:") totalMemory = parts[1].toULongLong();
        if (parts[0] == "MemAvailable:") available = parts[1].toULongLong();
    }
    if (totalMemory && available <= totalMemory) m_memory = qRound(100.0*(totalMemory-available)/totalMemory);
    if (m_metricsEnabled && m_cpu >= 0 && m_memory >= 0) enqueue("PC",QString("%1 %2").arg(m_cpu).arg(m_memory));
    emit changed();
#endif
}
bool CompanionTools::exportTimeline(const QUrl& destination) {
    if (!destination.isLocalFile()) return false;
    QSaveFile file(destination.toLocalFile());
    const auto bytes = QJsonDocument(QJsonArray::fromVariantList(m_timeline)).toJson();
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit();
}
