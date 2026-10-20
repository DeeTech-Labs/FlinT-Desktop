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
    implicitWidth: 224
    implicitHeight: compact ? 78 : 560
    Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.divider; visible: !navigation.compact }
    GridLayout {
        anchors.fill: parent
        anchors.margins: navigation.compact ? 12 : 18
        columns: navigation.compact ? 7 : 1
        rowSpacing: 7
        columnSpacing: 5
        RowLayout {
            Layout.fillWidth: true
            Layout.bottomMargin: navigation.compact ? 0 : 28
            spacing: 10
            Rectangle {
                width: navigation.compact ? 28 : 34; height: width; radius: 9
                color: Theme.accent
                AppIcon { anchors.centerIn: parent; width: 23; height: 23; name: "watch"; tint: "white" }
            }
            Label { visible: !navigation.compact; text: "FlinT"; font.pixelSize: 23; font.weight: Font.Bold; font.letterSpacing: 0.4; color: Theme.text }
        }
        Repeater {
            model: [qsTr("Overview"), qsTr("My watch"), qsTr("Watch faces"), "Companion"]
            delegate: NavigationButton {
                required property string modelData
                required property int index
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                compact: navigation.compact
                text: modelData
                iconName: ["home","watch","grid","companion"][index]
                selected: navigation.currentIndex === index
                onClicked: navigation.selected(index)
            }
        }
        Item { visible: !navigation.compact; Layout.fillHeight: true; Layout.minimumHeight: 24 }
        Repeater {
            model: [qsTr("Diagnostics"),qsTr("Settings")]
            delegate: NavigationButton {
                required property string modelData
                required property int index
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                compact: navigation.compact
                text: modelData
                iconName: index === 0 ? "activity" : "settings"
                selected: navigation.currentIndex === index + 4
                onClicked: navigation.selected(index + 4)
            }
        }
        Button {
            visible: !navigation.compact
            Layout.fillWidth: true
            Layout.topMargin: 22
            implicitHeight: 80
            padding: 12
            hoverEnabled: true
            onClicked: navigation.selected(6)
            background: Rectangle { radius: 11; color: parent.hovered ? Theme.buttonHover : Theme.surface; border.color: navigation.currentIndex === 6 ? Theme.accent : Theme.divider }
            contentItem: RowLayout {
                spacing: 10
                AppIcon { name: "watch"; tint: navigation.session.connected ? "#72a4ff" : Theme.muted; Layout.preferredWidth: 25; Layout.preferredHeight: 30 }
                ColumnLayout {
                    spacing: 4
                    Label { text: navigation.session.info.model || qsTr("No device selected"); textFormat: Text.PlainText; font.pixelSize: 12; font.weight: Font.DemiBold; Layout.fillWidth: true; elide: Text.ElideRight }
                    RowLayout {
                        spacing: 5
                        Rectangle { width: 5; height: 5; radius: 3; color: navigation.session.connected ? "#50d9b0" : Theme.offlineDot }
                        Label { text: navigation.session.connected ? navigation.session.transport : qsTr("Not connected"); color: Theme.muted; font.pixelSize: 10 }
                    }
                }
                AppIcon { name: "right"; width: 13; height: 13 }
            }
        }
        Label { visible: !navigation.compact; text: "DESKTOP  /  " + Qt.application.version; color: Theme.muted; font.pixelSize: 9; font.letterSpacing: 1.2; Layout.topMargin: 8; Layout.leftMargin: 4 }
    }
}
