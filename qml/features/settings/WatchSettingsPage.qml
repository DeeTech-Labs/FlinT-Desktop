import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Flint.Desktop

AppPage {
    id: page
    required property var service
    required property var session
    required property var presentation
    positionKey: "watch-" + tab
    property int tab: Theme.watchTab
    onTabChanged: Theme.watchTab = tab
    FileDialog { id: saveBackup; fileMode: FileDialog.SaveFile; nameFilters: ["JSON (*.json)"]; defaultSuffix: "json"; onAccepted: page.service.backup(selectedFile) }
    FileDialog { id: openBackup; fileMode: FileDialog.OpenFile; nameFilters: ["JSON (*.json)"]; onAccepted: page.service.restore(selectedFile) }
    SectionTabs {
        id: watchTabs
        Layout.fillWidth: true
        labels: [qsTr("Time and date"), qsTr("Display"), qsTr("Device"), qsTr("Profiles")]
        currentIndex: page.tab
        onCurrentIndexChanged: page.tab = currentIndex
    }
    AppCard {
        visible: page.tab === 0
        Layout.fillWidth: true
        implicitHeight: form.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: form
            anchors.fill: parent
            spacing: 12
            Label { text: qsTr("Time and date"); font.pixelSize: 21; font.weight: Font.DemiBold; Layout.fillWidth: true }
            Caption { text: page.service.supported ? page.service.state : qsTr("Connect a device with settings support, or start the simulator."); Layout.fillWidth: true }
            GridLayout {
                Layout.fillWidth: true
                columns: width < 600 ? 1 : 2
                columnSpacing: 18; rowSpacing: 14
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
                    Layout.preferredWidth: 1
                    Layout.minimumWidth: 0
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
                AppButton { iconName: "sync"; text: qsTr("Reload"); available: page.service.supported; onClicked: page.service.refresh() }
            }
        }
    }
    AppCard {
        visible: page.tab === 1
        Layout.fillWidth: true
        implicitHeight: appearance.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: appearance
            anchors.fill: parent
            spacing: 12
            Label { text: qsTr("Display"); font.pixelSize: 24; font.weight: Font.DemiBold }
            SettingChoice {
                service: page.service; setting: "language"; label: qsTr("Watch language")
                choices: [{label: "Русский", value: 0}, {label: "English", value: 1}]
            }
            Caption { text: qsTr("Brightness, screen timeout and orientation controls are unavailable in this firmware."); Layout.fillWidth: true }
        }
    }
    DeviceInfoCard { visible: page.tab === 2; session: page.session; presentation: page.presentation }
    AppCard {
        visible: page.tab === 2
        Layout.fillWidth: true
        implicitHeight: deviceTools.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: deviceTools
            anchors.fill: parent
            Caption { text: qsTr("Firmware updates are unavailable in this build."); Layout.fillWidth: true }
            AppButton { iconName: "save"; text: qsTr("Backup…"); available: page.service.supported; onClicked: saveBackup.open() }
            AppButton { iconName: "save"; text: qsTr("Restore…"); available: page.service.supported; onClicked: openBackup.open() }
        }
    }
    AppCard {
        visible: page.tab === 3
        Layout.fillWidth: true
        implicitHeight: profilesContent.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: profilesContent; anchors.fill: parent; spacing: 12
            Label { text: qsTr("Device profiles"); font.pixelSize: 20; font.weight: Font.DemiBold; Layout.fillWidth: true }
            AppComboBox { id: profile; visible: false; model: page.service.profiles; textRole: "name"; Layout.fillWidth: true }
            GridLayout {
                Layout.fillWidth: true
                columns: width >= 440 ? 2 : 1
                columnSpacing: 10; rowSpacing: 10
                Repeater {
                    model: page.service.profiles
                    delegate: Button {
                        required property var modelData
                        required property int index
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        implicitHeight: 82
                        padding: 14
                        hoverEnabled: true
                        onClicked: { profile.currentIndex = index; page.service.loadProfile(index) }
                        background: Rectangle { radius: 11; color: profile.currentIndex === index ? Theme.navSelected : parent.hovered ? Theme.buttonHover : Theme.input; border.color: profile.currentIndex === index ? Theme.accent : Theme.divider }
                        contentItem: RowLayout {
                            spacing: 12
                            AppIcon { name: index % 3 === 0 ? "companion" : index % 3 === 1 ? "moon" : "watch"; tint: "#82adff"; Layout.preferredWidth: 23; Layout.preferredHeight: 23 }
                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 5
                                Label { text: modelData.name; textFormat: Text.PlainText; font.pixelSize: 13; font.weight: Font.DemiBold; Layout.fillWidth: true; elide: Text.ElideRight }
                                Caption { text: qsTr("Prepare these settings"); Layout.fillWidth: true; font.pixelSize: 11 }
                            }
                        }
                    }
                }
            }
            AppTextField { id: profileName; placeholderText: qsTr("New profile name"); maximumLength: 64; Layout.fillWidth: true }
            Flow {
                Layout.fillWidth: true; Layout.preferredHeight: childrenRect.height; spacing: 10
                AppButton { iconName: "save"; text: qsTr("Save profile"); available: page.service.supported && profileName.text.trim().length > 0; onClicked: {page.service.saveProfile(profileName.text);profileName.clear()} }
                AppButton { iconName: "close"; text: qsTr("Delete profile"); available: profile.count > 0; onClicked: page.service.deleteProfile(profile.currentIndex) }
            }
            Caption { text: qsTr("Loading a profile prepares changes. Apply them to write to the watch."); Layout.fillWidth: true }
        }
    }
}
