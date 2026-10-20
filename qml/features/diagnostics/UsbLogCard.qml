import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

AppCard {
    required property var session
    implicitHeight: content.implicitHeight + topPadding + bottomPadding
    Layout.fillWidth: true
    ColumnLayout {
        id: content
        anchors.fill: parent
        spacing: 16
        RowLayout {
            Label {
                text: qsTr("Connection log")
                font.pixelSize: 20
                font.bold: true
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            AppButton {
                text: qsTr("Clear")
                onClicked: session.clearLog()
            }
        }
        Caption {
            text: qsTr("Connection events and data exchanged with the clock.")
            Layout.fillWidth: true
        }
        AppTextField {
            id: command
            Layout.fillWidth: true
            placeholderText: qsTr("Protocol operation, for example SETTINGS")
            maximumLength: 280
            onAccepted: { if (send.available) send.clicked() }
        }
        AppButton {
            id: send
            text: qsTr("Send protocol request")
            available: session.connected && !session.busy && (session.info.capabilities || []).indexOf("control") >= 0 && command.text.trim().length > 0
            onClicked: {
                const text = command.text.trim()
                const space = text.indexOf(" ")
                session.request(space < 0 ? text : text.substring(0,space),space < 0 ? "" : text.substring(space+1))
            }
        }
        LogView {
            Layout.fillWidth: true
            Layout.preferredHeight: 380
            text: session.log
            placeholderText: qsTr("USB events will appear here…")
        }
    }
}
