import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

AppCard {
    required property var session
    required property var presentation
    signal exportRequested
    implicitHeight: content.implicitHeight + topPadding + bottomPadding
    Layout.fillWidth: true
    ColumnLayout {
        id: content
        anchors.fill: parent
        spacing: 14
        Label {
            text: qsTr("About the device")
            font.pixelSize: 20
            font.bold: true
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Label {
            text: "ID: " + presentation.value(session.info, "id") + "  ·  " + presentation.value(session.info, "board") + "  ·  " + presentation.value(session.info, "chip")
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Caption {
            text: qsTr("Capabilities: ") + (presentation.capabilities() || "—")
            Layout.fillWidth: true
        }
        GridLayout {
            Layout.fillWidth: true
            columns: width < 440 ? 1 : width < 740 ? 2 : 3
            columnSpacing: 10
            rowSpacing: 10
            MetricCard {
                Layout.fillWidth: true
                label: qsTr("Clock language")
                detail: presentation.setting("language", {
                    0: qsTr("Russian"),
                    1: "English"
                })
            }
            MetricCard {
                Layout.fillWidth: true
                label: qsTr("Time format")
                detail: presentation.setting("hour_format", {
                    0: qsTr("24 hours"),
                    1: qsTr("12 hours")
                })
            }
            MetricCard {
                Layout.fillWidth: true
                label: qsTr("Show seconds")
                detail: presentation.setting("show_seconds", {
                    0: qsTr("Off"),
                    1: qsTr("On"),
                    255: qsTr("Automatic")
                })
            }
        }
    }
}
