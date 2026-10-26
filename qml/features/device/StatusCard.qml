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
        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Clock status")
                font.pixelSize: 20
                font.bold: true
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Caption {
                text: session.connected ? qsTr("●  Every 2 seconds") : qsTr("○  Offline")
            }
        }
        Caption {
            visible: session.sampleCount === 0
            text: qsTr("Data will appear here after you connect your clock.")
            Layout.fillWidth: true
        }
        Caption {
            visible: session.sampleCount > 0 && !session.connected
            text: qsTr("Showing the last received data.")
            Layout.fillWidth: true
        }
        GridLayout {
            Layout.fillWidth: true
            columns: width < 440 ? 1 : width < 740 ? 2 : 3
            columnSpacing: 10
            rowSpacing: 10
            MetricCard {
                Layout.fillWidth: true
                label: qsTr("Device time")
                detail: session.sampleCount ? presentation.clockValue() : "—"
            }
            MetricCard {
                Layout.fillWidth: true
                label: qsTr("Uptime")
                detail: presentation.uptime()
            }
            MetricCard {
                Layout.fillWidth: true
                label: qsTr("Free memory")
                detail: session.status.free_heap_bytes === undefined ? "—" : qsTr("%1 KiB").arg(Math.round(session.status.free_heap_bytes / 1024))
            }
            MetricCard {
                Layout.fillWidth: true
                label: qsTr("Display time zone")
                detail: presentation.value(session.status, "timezone")
            }
            MetricCard {
                Layout.fillWidth: true
                label: qsTr("Watch face")
                detail: presentation.value(session.status, "active_face")
            }
            MetricCard {
                Layout.fillWidth: true
                label: qsTr("PC synchronization")
                detail: !session.sampleCount ? "—" : session.status.pc_synced ? qsTr("Completed") : qsTr("Awaiting synchronization")
            }
        }
        Flow {
            Layout.fillWidth: true
            Layout.preferredHeight: childrenRect.height
            spacing: 10
            AppButton {
                busy: session.interactionBusy
                text: qsTr("Refresh now")
                available: session.connected
                onClicked: session.refreshStatus()
            }
            AppButton {
                text: qsTr("Save JSON…")
                available: session.sampleCount > 0
                onClicked: exportRequested()
            }
        }
        Caption {
            text: qsTr("Snapshots: ") + session.sampleCount + " / 600 · " + (session.lastUpdated || qsTr("No data received yet"))
            Layout.fillWidth: true
        }
    }
}
