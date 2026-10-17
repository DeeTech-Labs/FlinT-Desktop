import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

Rectangle {
    id: navigation
    required property var session
    property int currentIndex: 0
    property bool compact: false
    signal selected(int index)
    color: Theme.sidebar
    implicitWidth: 218
    implicitHeight: compact ? 64 : 400
    GridLayout {
        anchors.fill: parent
        anchors.margins: navigation.compact ? 8 : 20
        columns: navigation.compact ? 4 : 1
        rowSpacing: 10
        Label {
            text: "flinT."
            color: Theme.accent
            font.pixelSize: navigation.compact ? 24 : 38
            font.bold: true
            Layout.bottomMargin: navigation.compact ? 0 : 32
        }
        Repeater {
            model: [qsTr("Overview"), qsTr("Clock data"), qsTr("USB log")]
            delegate: NavigationButton {
                required property string modelData
                required property int index
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: modelData
                selected: navigation.currentIndex === index
                onClicked: navigation.selected(index)
            }
        }
        Item {
            visible: !navigation.compact
            Layout.fillHeight: true
        }
        Caption {
            visible: !navigation.compact
            text: navigation.session.connected ? qsTr("Clock connected") : qsTr("Not connected")
            Layout.fillWidth: true
        }
        Caption {
            visible: !navigation.compact
            text: "FlinT Desktop / " + Qt.application.version
            font.pixelSize: 10
        }
    }
}
