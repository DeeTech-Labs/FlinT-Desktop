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
                text: qsTr("USB log")
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
        LogView {
            Layout.fillWidth: true
            Layout.preferredHeight: 380
            text: session.log
            placeholderText: qsTr("USB events will appear here…")
        }
    }
}
