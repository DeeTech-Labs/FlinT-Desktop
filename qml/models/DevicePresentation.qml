import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

QtObject {
    required property var session
    function value(object, key) {
        return object[key] === undefined ? "—" : String(object[key]);
    }
    function clockValue() {
        if (!session.status.time_valid)
            return qsTr("Time is not set");
        return session.formatTime(session.status.epoch, session.status.timezone, "dd.MM.yyyy HH:mm:ss");
    }
    function uptime() {
        if (session.status.uptime_ms === undefined)
            return "—";
        let seconds = Math.floor(session.status.uptime_ms / 1000);
        return qsTr("%1 h %2 min %3 s").arg(Math.floor(seconds / 3600)).arg(Math.floor(seconds % 3600 / 60)).arg(seconds % 60);
    }
    function capabilities() {
        const names = {
            info: qsTr("device information"),
            status: qsTr("status collection"),
            time_sync: qsTr("time synchronization")
        };
        return (session.info.capabilities || []).filter(key => names[key] !== undefined).map(key => names[key]).join(", ");
    }
    function setting(name, labels) {
        const settings = session.status.settings || {};
        return settings[name] === undefined ? "—" : labels[settings[name]] || qsTr("Other");
    }
}
