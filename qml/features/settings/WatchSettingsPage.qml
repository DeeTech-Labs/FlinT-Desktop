import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Flint.Desktop

AppPage {
    id: page
    required property var service
    FileDialog { id: saveBackup; fileMode: FileDialog.SaveFile; nameFilters: ["JSON (*.json)"]; defaultSuffix: "json"; onAccepted: page.service.backup(selectedFile) }
    FileDialog { id: openBackup; fileMode: FileDialog.OpenFile; nameFilters: ["JSON (*.json)"]; onAccepted: page.service.restore(selectedFile) }
    AppCard {
        Layout.fillWidth: true
        implicitHeight: form.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: form
            anchors.fill: parent
            spacing: 16
            Label { text: qsTr("My watch"); font.pixelSize: 24; font.bold: true; Layout.fillWidth: true }
            Caption { text: page.service.supported ? page.service.state : qsTr("Connect a device with settings support, or start the simulator."); Layout.fillWidth: true }
            GridLayout {
                Layout.fillWidth: true
                columns: width < 600 ? 1 : 2
                columnSpacing: 18; rowSpacing: 14
                SettingChoice {
                    service: page.service; setting: "language"; label: qsTr("Watch language")
                    choices: [{label: "Русский", value: 0}, {label: "English", value: 1}]
                }
                SettingChoice {
                    service: page.service; setting: "hour_format"; label: qsTr("Time format")
                    choices: [{label: qsTr("24 hours"), value: 0}, {label: qsTr("12 hours"), value: 1}]
                }
                SettingChoice {
                    service: page.service; setting: "show_seconds"; label: qsTr("Seconds")
                    choices: [{label: qsTr("Automatic"), value: 255}, {label: qsTr("Show"), value: 1}, {label: qsTr("Hide"), value: 0}]
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    Caption { text: qsTr("Time zone") }
                    AppComboBox {
                        Layout.fillWidth: true
                        Accessible.name: qsTr("Time zone")
                        model: page.service.timezones
                        currentIndex: page.service.timezones.indexOf(page.service.draft.timezone || "")
                        enabled: page.service.supported
                        onActivated: page.service.edit("timezone",currentText)
                    }
                }
                SettingChoice {
                    service: page.service; setting: "date_format"; label: qsTr("Date format")
                    choices: [{label: "31.12.26",value:0},{label:"31.12.2026",value:1},{label:"12.31.26",value:2},{label:"12.31.2026",value:3},{label:"2026.12.31",value:4},{label:qsTr("Month name"),value:5}]
                }
                SettingChoice {
                    service: page.service; setting: "date_separator"; label: qsTr("Date separator")
                    choices: [{label: ".",value:0},{label:"/",value:1},{label:"-",value:2}]
                }
            }
            Flow {
                Layout.fillWidth: true
                Layout.preferredHeight: childrenRect.height
                spacing: 10
                AppButton { text: qsTr("Apply changes"); primary: true; available: page.service.supported && page.service.dirty; onClicked: page.service.apply() }
                AppButton { text: qsTr("Discard"); available: page.service.dirty; onClicked: page.service.discard() }
                AppButton { text: qsTr("Reload"); available: page.service.supported; onClicked: page.service.refresh() }
                AppButton { text: qsTr("Backup…"); available: page.service.supported; onClicked: saveBackup.open() }
                AppButton { text: qsTr("Restore…"); available: page.service.supported; onClicked: openBackup.open() }
            }
            Caption { text: qsTr("Display brightness requires controllable backlight hardware and is not advertised by this board."); Layout.fillWidth: true }
        }
    }
    AppCard {
        Layout.fillWidth: true
        implicitHeight: profilesContent.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: profilesContent; anchors.fill: parent; spacing: 12
            Label { text: qsTr("Device profiles"); font.pixelSize: 20; font.bold: true; Layout.fillWidth: true }
            AppComboBox { id: profile; model: page.service.profiles; textRole: "name"; Layout.fillWidth: true }
            TextField { id: profileName; placeholderText: qsTr("New profile name"); maximumLength: 64; Layout.fillWidth: true }
            Flow {
                Layout.fillWidth: true; Layout.preferredHeight: childrenRect.height; spacing: 10
                AppButton { text: qsTr("Load profile"); available: page.service.supported && profile.count > 0; onClicked: page.service.loadProfile(profile.currentIndex) }
                AppButton { text: qsTr("Save profile"); available: page.service.supported && profileName.text.trim().length > 0; onClicked: {page.service.saveProfile(profileName.text);profileName.clear()} }
                AppButton { text: qsTr("Delete profile"); available: profile.count > 0; onClicked: page.service.deleteProfile(profile.currentIndex) }
            }
            Caption { text: qsTr("Loading a profile prepares changes. Apply them to write to the watch."); Layout.fillWidth: true }
        }
    }
}
