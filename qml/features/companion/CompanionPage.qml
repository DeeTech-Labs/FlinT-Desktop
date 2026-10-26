import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Flint.Desktop

AppPage {
    id: page
    positionKey: "companion-" + tab
    required property var service
    property int tab: Theme.companionTab
    onTabChanged: Theme.companionTab = tab
    FileDialog { id: timelineFile; fileMode: FileDialog.SaveFile; defaultSuffix: "json"; nameFilters: ["JSON (*.json)"]; onAccepted: page.service.exportTimeline(selectedFile) }
    SectionTabs {
        id: companionTabs
        Layout.fillWidth: true
        labels: [qsTr("Notes"), qsTr("Focus"), qsTr("System"), qsTr("Integrations"), qsTr("Timeline")]
        currentIndex: page.tab
        onCurrentIndexChanged: page.tab = currentIndex
    }
    AppCard {
        visible: page.tab === 3
        Layout.fillWidth: true
        implicitHeight: actionsContent.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: actionsContent
            anchors.fill: parent
            spacing: 12
            Label { text: "FlinT Actions"; font.pixelSize: 24; font.weight: Font.DemiBold }
            Caption { text: qsTr("Buttons are emulated through the serial monitor on this board."); Layout.fillWidth: true }
            AppSwitch {
                text: qsTr("Enable actions for this connection")
                enabled: page.service.actionsAvailable
                checked: page.service.actionsArmed
                onToggled: page.service.actionsArmed = checked
            }
            Repeater {
                model: ["primary", "secondary", "long"]
                delegate: ColumnLayout {
                    required property string modelData
                    Layout.fillWidth: true
                    Caption { text: modelData === "primary" ? qsTr("Primary button") : modelData === "secondary" ? qsTr("Secondary button") : qsTr("Long press") }
                    AppComboBox {
                        Layout.fillWidth: true
                        property var keys: ["none", "play_pause", "next", "previous", "focus", "focus_stop", "presentation_next", "presentation_previous"]
                        model: [qsTr("No action"), qsTr("Play / pause"), qsTr("Next track"), qsTr("Previous track"), qsTr("Focus · 25 minutes"), qsTr("Stop focus"), qsTr("Next slide (X11)"), qsTr("Previous slide (X11)")]
                        currentIndex: Math.max(0, keys.indexOf(page.service.actionMappings[modelData] || "none"))
                        onActivated: page.service.setAction(modelData, keys[currentIndex])
                    }
                    AppButton { text: qsTr("Emulate button via USB / simulator"); available: page.service.inputEmulationAvailable; onClicked: page.service.emulateButton(modelData) }
                }
            }
        }
    }
    AppCard {
        visible: page.tab === 2
        Layout.fillWidth: true
        implicitHeight: content.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: content; anchors.fill: parent; spacing: 14
            Label { text: "PC Companion"; font.pixelSize: 24; font.weight: Font.DemiBold; Layout.fillWidth: true }
            Caption { text: page.service.supported ? page.service.state : qsTr("Connect a device with companion support."); Layout.fillWidth: true }
            AppSwitch { Layout.fillWidth: true; Layout.minimumWidth: 0; Component.onCompleted: contentItem.wrapMode = Text.WordWrap; text: qsTr("Send computer CPU and RAM to watch"); enabled: page.service.metricsAvailable; checked: page.service.metricsEnabled; onToggled: page.service.metricsEnabled = checked }
            Caption { text: page.service.metricsAvailable ? qsTr("CPU: %1% · RAM: %2%").arg(page.service.cpu < 0 ? "—" : page.service.cpu).arg(page.service.memory < 0 ? "—" : page.service.memory) : qsTr("System metrics are currently available on Linux."); Layout.fillWidth: true }
            AppButton { iconName: "search"; text: qsTr("Find watch"); available: page.service.supported; onClicked: page.service.findWatch() }
            Caption { text: qsTr("Find watch flashes the board indicator for five seconds."); Layout.fillWidth: true }
        }
    }
    AppCard {
        visible: page.tab === 1
        Layout.fillWidth: true
        implicitHeight: focusContent.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: focusContent; anchors.fill: parent; spacing: 14
            Label { text: qsTr("Focus session"); font.pixelSize: 24; font.weight: Font.DemiBold; Layout.fillWidth: true }
            Item {
                Layout.fillWidth: true
                implicitHeight: 164
                AmbientAccent {
                    objectName: "focusAccent"
                    anchors.fill: parent
                    kind: 0
                    active: page.tab === 1 && (!page.inSwipeView || page.currentSwipePage)
                    moving: page.service.focusRemaining > 0
                    strength: moving ? 1.0 : 0.45
                }
                Label {
                    anchors.centerIn: parent
                    text: Math.floor(page.service.focusRemaining / 60).toString().padStart(2,"0") + ":" + (page.service.focusRemaining % 60).toString().padStart(2,"0")
                    font.pixelSize: parent.width < 420 ? 52 : 62
                    font.family: Theme.monoFamily; font.weight: Font.Light
                    color: Theme.text
                }
            }
            Flow {
                Layout.fillWidth: true; Layout.preferredHeight: childrenRect.height; spacing: 8
                Repeater {
                    model: [15, 25, 45, 60]
                    delegate: AppButton {
                        required property int modelData
                        text: qsTr("%1 min").arg(modelData)
                        primary: minutes.value === modelData
                        onClicked: minutes.value = modelData
                    }
                }
            }
            SpinBox { id: minutes; from: 1; to: 1440; value: 25; editable: true; Accessible.name: qsTr("Focus minutes") }
            Flow {
                Layout.fillWidth: true; Layout.preferredHeight: childrenRect.height; spacing: 10
                AppButton { iconName: "play"; text: qsTr("Start focus"); primary: true; available: page.service.supported; onClicked: page.service.startFocus(minutes.value) }
                AppButton { iconName: "close"; text: qsTr("Stop focus"); available: page.service.supported && page.service.focusRemaining > 0; onClicked: page.service.stopFocus() }
            }
            Caption { text: qsTr("The watch timer keeps running when the computer disconnects."); Layout.fillWidth: true }
        }
    }
    AppCard {
        visible: page.tab === 0
        Layout.fillWidth: true
        implicitHeight: noteContent.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: noteContent; anchors.fill: parent; spacing: 14
            Label { text: qsTr("Quick note"); font.pixelSize: 24; font.weight: Font.DemiBold; Layout.fillWidth: true }
            AppTextField { id: note; objectName: "noteEditor"; property int utf8Length: encodeURIComponent(text).replace(/%[A-F\d]{2}/gi, "x").length; text: page.service.note; placeholderText: qsTr("Short text for your watch"); maximumLength: 96; Layout.fillWidth: true }
            Flow {
                Layout.fillWidth: true; Layout.preferredHeight: childrenRect.height; spacing: 10
                AppButton { iconName: "note"; text: qsTr("Send note"); primary: true; available: page.service.supported && note.text.trim().length > 0 && note.utf8Length <= 96; onClicked: page.service.sendNote(note.text) }
                AppButton { iconName: "note"; text: qsTr("Send clipboard text"); available: page.service.supported; onClicked: page.service.sendClipboard() }
            }
            Caption { text: qsTr("%1 / %2 bytes").arg(note.utf8Length).arg(96); color: note.utf8Length > 96 ? Theme.error : Theme.muted; Layout.fillWidth: true }
            Caption { text: qsTr("Notes are stored on the watch. Maximum size is 96 UTF-8 bytes."); Layout.fillWidth: true }
        }
    }
    AppCard {
        visible: page.tab === 4
        Layout.fillWidth: true
        implicitHeight: history.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: history; anchors.fill: parent; spacing: 10
            Label { text: qsTr("Device timeline"); font.pixelSize: 24; font.weight: Font.DemiBold; Layout.fillWidth: true }
            AppButton { text: qsTr("Export timeline…"); onClicked: timelineFile.open() }
            Caption { visible: !page.service.timeline.length; text: qsTr("Your device activity will appear here."); Layout.fillWidth: true }
            Repeater {
                model: page.service.timeline.slice(-12).reverse()
                delegate: Caption { required property var modelData; text: modelData.at + " · " + modelData.event; textFormat: Text.PlainText; Layout.fillWidth: true }
            }
        }
    }
}
