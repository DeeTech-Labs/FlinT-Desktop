import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Flint.Desktop

AppPage {
    id: page
    required property var service
    FileDialog { id: timelineFile; fileMode: FileDialog.SaveFile; defaultSuffix: "json"; nameFilters: ["JSON (*.json)"]; onAccepted: page.service.exportTimeline(selectedFile) }
    AppCard {
        Layout.fillWidth: true
        implicitHeight: content.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: content; anchors.fill: parent; spacing: 14
            Label { text: "PC Companion"; font.pixelSize: 24; font.bold: true; Layout.fillWidth: true }
            Caption { text: page.service.supported ? page.service.state : qsTr("Connect a device with companion support."); Layout.fillWidth: true }
            CheckBox { Layout.fillWidth: true; Layout.minimumWidth: 0; Component.onCompleted: contentItem.wrapMode = Text.WordWrap; text: qsTr("Send computer CPU and RAM to watch"); enabled: page.service.metricsAvailable; checked: page.service.metricsEnabled; onToggled: page.service.metricsEnabled = checked }
            Caption { text: page.service.metricsAvailable ? qsTr("CPU: %1% · RAM: %2%").arg(page.service.cpu < 0 ? "—" : page.service.cpu).arg(page.service.memory < 0 ? "—" : page.service.memory) : qsTr("System metrics are currently available on Linux."); Layout.fillWidth: true }
            AppButton { text: qsTr("Find watch"); available: page.service.supported; onClicked: page.service.findWatch() }
            Caption { text: qsTr("Find watch flashes the board indicator for five seconds."); Layout.fillWidth: true }
        }
    }
    AppCard {
        Layout.fillWidth: true
        implicitHeight: focusContent.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: focusContent; anchors.fill: parent; spacing: 14
            Label { text: qsTr("Focus session"); font.pixelSize: 24; font.bold: true; Layout.fillWidth: true }
            Label { text: Math.floor(page.service.focusRemaining / 60).toString().padStart(2,"0") + ":" + (page.service.focusRemaining % 60).toString().padStart(2,"0"); font.pixelSize: 42; color: Theme.accent; Layout.fillWidth: true }
            SpinBox { id: minutes; from: 1; to: 1440; value: 25; editable: true; Accessible.name: qsTr("Focus minutes") }
            Flow {
                Layout.fillWidth: true; Layout.preferredHeight: childrenRect.height; spacing: 10
                AppButton { text: qsTr("Start focus"); primary: true; available: page.service.supported; onClicked: page.service.startFocus(minutes.value) }
                AppButton { text: qsTr("Stop focus"); available: page.service.supported; onClicked: page.service.stopFocus() }
            }
            Caption { text: qsTr("The watch timer keeps running when the computer disconnects."); Layout.fillWidth: true }
        }
    }
    AppCard {
        Layout.fillWidth: true
        implicitHeight: noteContent.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: noteContent; anchors.fill: parent; spacing: 14
            Label { text: qsTr("Quick note"); font.pixelSize: 24; font.bold: true; Layout.fillWidth: true }
            TextField { id: note; text: page.service.note; placeholderText: qsTr("Short text for your watch"); maximumLength: 96; Layout.fillWidth: true }
            Flow {
                Layout.fillWidth: true; Layout.preferredHeight: childrenRect.height; spacing: 10
                AppButton { text: qsTr("Send note"); available: page.service.supported; onClicked: page.service.sendNote(note.text) }
                AppButton { text: qsTr("Send clipboard text"); available: page.service.supported; onClicked: page.service.sendClipboard() }
            }
            Caption { text: qsTr("Notes are stored on the watch. Maximum size is 96 UTF-8 bytes."); Layout.fillWidth: true }
        }
    }
    AppCard {
        Layout.fillWidth: true
        implicitHeight: history.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: history; anchors.fill: parent; spacing: 10
            Label { text: qsTr("Device timeline"); font.pixelSize: 24; font.bold: true; Layout.fillWidth: true }
            AppButton { text: qsTr("Export timeline…"); onClicked: timelineFile.open() }
            Repeater {
                model: page.service.timeline.slice(-12).reverse()
                delegate: Caption { required property var modelData; text: modelData.at + " · " + modelData.event; textFormat: Text.PlainText; Layout.fillWidth: true }
            }
        }
    }
}
