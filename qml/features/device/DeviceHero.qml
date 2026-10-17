import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

Rectangle {
    required property var session
    required property var presentation

    implicitHeight: Math.max(222, heroContent.implicitHeight + 56)
    radius: 22
    clip: true
    gradient: Gradient {
        orientation: Gradient.Horizontal
        GradientStop {
            position: 0
            color: Theme.heroStart
        }
        GradientStop {
            position: 1
            color: Theme.heroEnd
        }
    }
    Rectangle {
        width: 240
        height: 240
        radius: 120
        x: parent.width - 240
        y: 40
        color: Theme.heroDecoration
        opacity: 0.5
    }
    Rectangle {
        visible: parent.width >= 470
        width: 138
        height: 154
        radius: 32
        rotation: 10
        anchors.right: parent.right
        anchors.rightMargin: 42
        anchors.verticalCenter: parent.verticalCenter
        color: Theme.watchBody
        border.color: Theme.watchBorder
        border.width: 3
        Rectangle {
            anchors.fill: parent
            anchors.margins: 10
            radius: 23
            color: Theme.watchScreen
            Column {
                anchors.centerIn: parent
                spacing: 2
                Label {
                    text: session.sampleCount > 0 && session.status.time_valid ? session.formatTime(session.status.epoch, session.status.timezone, "HH:mm") : "--:--"
                    font.pixelSize: 34
                    font.bold: true
                    color: Theme.accent
                }
                Label {
                    text: "F L I N T   O S"
                    color: Theme.watchCaption
                    font.pixelSize: 9
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }
    }
    ColumnLayout {
        id: heroContent
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 28
        width: parent.width - (parent.width < 470 ? 56 : 240)
        spacing: 10
        Caption {
            text: session.connected ? qsTr("CONNECTED VIA USB") : qsTr("YOUR RHYTHM. YOUR TIME.")
            color: Theme.heroCaption
            font.pixelSize: 10
            font.letterSpacing: 1.5
        }
        Label {
            text: session.info.model === undefined ? "FlinT Clock" : String(session.info.model)
            textFormat: Text.PlainText
            font.pixelSize: 34
            font.bold: true
            Layout.fillWidth: true
            elide: Text.ElideRight
        }
        Label {
            text: session.connected ? qsTr("Clock status and accurate time in one place.") : qsTr("Connect your clock to sync the time and view its status.")
            color: Theme.heroText
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            font.pixelSize: 13
        }
        Item {
            Layout.fillHeight: true
        }
        Caption {
            text: session.info.firmware === undefined ? "FlinT OS  ·  USB Companion" : "FlinT OS " + presentation.value(session.info, "firmware")
            color: Theme.heroFooter
        }
    }
}
