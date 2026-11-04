import QtQuick

QtObject {
    required property var session
    readonly property string stateLabel: session.status
    function value(object, key) {
        return object[key] === undefined ? "—" : String(object[key]);
    }
}
