import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

Item {
    id: navigation
    required property var session
    property int currentIndex: 0
    property bool compact: false
    signal selected(int index)
    implicitWidth: 60
    implicitHeight: compact ? 48 : 360
    GridLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 5
        columns: navigation.compact ? 8 : 1
        rowSpacing: 4; columnSpacing: 3
        Label {
            text: "f."
            font.pixelSize: 28; font.weight: Font.Black; font.letterSpacing: -2
            color: Theme.accent
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            Layout.bottomMargin: navigation.compact ? 0 : 8
        }
        Repeater {
            model: [qsTr("Overview"), qsTr("My watch"), qsTr("Watch faces"), "Companion"]
            delegate: NavigationButton {
                required property string modelData
                required property int index
                Layout.fillWidth: true; Layout.minimumWidth: 0
                compact: true
                text: modelData
                iconName: ["home", "watch", "grid", "companion"][index]
                selected: navigation.currentIndex === index
                onClicked: navigation.selected(index)
            }
        }
        Rectangle {
            visible: !navigation.compact
            Layout.fillWidth: true
            Layout.leftMargin: 10; Layout.rightMargin: 10
            Layout.topMargin: 5; Layout.bottomMargin: 5
            height: 1; color: Theme.divider
        }
        Repeater {
            model: [qsTr("Diagnostics"), qsTr("Settings"), qsTr("Connection")]
            delegate: NavigationButton {
                required property string modelData
                required property int index
                readonly property int pageIndex: [4, 5, 6][index]
                Layout.fillWidth: true; Layout.minimumWidth: 0
                compact: true
                text: modelData
                iconName: ["activity", "settings", "connection"][index]
                selected: navigation.currentIndex === pageIndex
                onClicked: navigation.selected(pageIndex)
            }
        }
    }
}
