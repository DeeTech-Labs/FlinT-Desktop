#include "SimulatedDevice.h"
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>
namespace {
QByteArray json(const QJsonObject& object) { return QJsonDocument(object).toJson(QJsonDocument::Compact); }
}
SimulatedDevice::SimulatedDevice() : m_settings{{"show_seconds",255},{"language",0},{"hour_format",0},{"date_format",0},{"date_separator",0},{"timezone","Europe/Moscow"},{"active_face","digital"}} {}
QByteArray SimulatedDevice::exchange(const QByteArray& frame) {
    const auto command = frame.trimmed();
    if (command == "@HELLO") return "FLINT HELLO 1 simulator\n";
    if (command == "@INFO") return "FLINT INFO " + json({{"protocol",1},{"id","simulator"},{"model","Spark Simulator"},{"board","simulator"},{"chip","virtual"},{"firmware","development"},{"api",1},{"screen",QJsonObject{{"width",280},{"height",240}}},{"capabilities",QJsonArray{"info","status","time_sync","control","settings","faces","face_studio","companion"}}}) + '\n';
    if (command == "@STATUS") {
        QJsonObject status{{"uptime_ms",QDateTime::currentMSecsSinceEpoch() % 86400000},{"free_heap_bytes",180000},{"usb_connected",false},{"time_valid",true},{"pc_synced",true},{"epoch",QDateTime::currentSecsSinceEpoch()+m_clockOffset},{"timezone",m_settings.value("timezone")},{"active_face",m_settings.value("active_face")},{"settings",m_settings}};
        return "FLINT STATUS " + json(status) + '\n';
    }
    if (command.startsWith("@TIME ")) {
        bool valid; const auto epoch = command.mid(6).toLongLong(&valid);
        if (!valid || epoch < 1700000000LL || epoch > 4102444799LL) return "FLINT ERROR invalid command\n";
        m_clockOffset = epoch - QDateTime::currentSecsSinceEpoch();
        return "FLINT TIME OK " + QByteArray::number(epoch) + '\n';
    }
    const auto match = QRegularExpression("^@REQ ([1-9][0-9]{0,8}) ([A-Z]+)(?: (.*))?$").match(QString::fromLatin1(command));
    if (!match.hasMatch()) return "FLINT ERROR invalid command\n";
    QJsonObject response{{"id",match.captured(1).toInt()},{"ok",true}};
    const auto operation = match.captured(2), args = match.captured(3);
    QString error;
    QJsonValue result;
    if (operation == "SETTINGS") result = m_settings;
    else if (operation == "TIMEZONES") result = QJsonArray{"Europe/Moscow","UTC","Europe/Berlin","Asia/Tokyo","America/New_York"};
    else if (operation == "FACES") result = QJsonArray{QJsonObject{{"id","digital"},{"name","Digital"},{"author","DeeTech Labs"},{"version",1},{"min_api",1}},QJsonObject{{"id","studio"},{"name","Face Studio"},{"author","DeeTech Labs"},{"version",1},{"min_api",1}},QJsonObject{{"id","companion"},{"name","PC Companion"},{"author","DeeTech Labs"},{"version",1},{"min_api",1}}};
    else if (operation == "FACE") {
        if (args != "digital" && args != "studio" && args != "companion") error = "unknown_face";
        else { m_settings.insert("active_face",args); result = m_settings; }
    } else if (operation == "APPS") result = QJsonObject{{"note",m_note},{"focus_remaining",std::max<qint64>(0,(m_focusDeadline-QDateTime::currentMSecsSinceEpoch()+999)/1000)}};
    else if (operation == "NOTE") {
        const auto bytes = QByteArray::fromHex(args.toLatin1());
        if (args.size() % 2 || bytes.size() > 96 || std::any_of(bytes.begin(),bytes.end(),[](char value) { return static_cast<unsigned char>(value) < 32; }) || !QRegularExpression("^[0-9a-fA-F]*$").match(args).hasMatch()) error = "invalid_note";
        else { m_note = QString::fromUtf8(bytes); result = QJsonObject{{"accepted",true}}; }
    } else if (operation == "FOCUS") {
        bool okay; const auto seconds = args.toInt(&okay);
        if (!okay || !QRegularExpression("^[0-9]+$").match(args).hasMatch() || seconds < 0 || seconds > 86400) error = "invalid_duration";
        else { m_focusDeadline = QDateTime::currentMSecsSinceEpoch()+seconds*1000LL; result = QJsonObject{{"accepted",true}}; }
    } else if (operation == "FIND") {
        if (!args.isEmpty()) error = "invalid_arguments"; else result = QJsonObject{{"accepted",true}};
    } else if (operation == "SHOW") {
        if (args != "note" && args != "focus" && args != "computer") error = "invalid_mode"; else result = QJsonObject{{"accepted",true}};
    } else if (operation == "PC") {
        const auto fields = args.split(' ');
        if (fields.size() != 2 || !QRegularExpression("^[0-9]{1,3} [0-9]{1,3}$").match(args).hasMatch() || fields[0].toInt() > 100 || fields[1].toInt() > 100) error = "invalid_metrics";
        else result = QJsonObject{{"accepted",true}};
    }
    else if (operation == "STUDIO") {
        const auto fields = args.split(' ',Qt::SkipEmptyParts);
        bool valid = fields.size() == 6;
        if (valid) valid = QRegularExpression("^[0-9a-fA-F]{6}$").match(fields[0]).hasMatch()
            && QRegularExpression("^[0-9a-fA-F]{6}$").match(fields[1]).hasMatch()
            && QRegularExpression("^[0-9a-fA-F]{64}$").match(fields[5]).hasMatch()
            && fields[2].toInt() >= 80 && fields[2].toInt() <= 150 && fields[3].toInt() >= 170 && fields[3].toInt() <= 220
            && (fields[4] == "0" || fields[4] == "1");
        if (!valid) error = "invalid_definition_or_storage";
        else result = QJsonObject{{"installed",true}};
    } else if (operation == "SET") {
        auto next = m_settings;
        const auto values = args.split(' ', Qt::SkipEmptyParts);
        QSet<QString> seen;
        for (const auto& value : values) {
            const auto separator = value.indexOf('=');
            const auto key = value.left(separator), text = value.mid(separator+1);
            bool valid = separator > 0 && next.contains(key) && key != "active_face" && !seen.contains(key);
            seen.insert(key);
            if (key == "timezone") {
                valid = valid && QJsonArray{"Europe/Moscow","UTC","Europe/Berlin","Asia/Tokyo","America/New_York"}.contains(text);
                next.insert(key,text);
            } else {
                bool number; const auto n = text.toInt(&number);
                valid = valid && number && (key == "show_seconds" ? n == 0 || n == 1 || n == 255 : n >= 0 && n <= (key == "date_format" ? 5 : key == "date_separator" ? 2 : 1));
                next.insert(key,n);
            }
            if (!valid) { error = "invalid_setting"; break; }
        }
        if (values.isEmpty()) error = "invalid_arguments";
        if (error.isEmpty()) { m_settings = next; result = m_settings; }
    } else error = "unsupported_operation";
    if (!error.isEmpty()) { response.insert("ok",false); response.insert("error",error); }
    else response.insert("result",result);
    return "FLINT RESULT " + json(response) + '\n';
}
