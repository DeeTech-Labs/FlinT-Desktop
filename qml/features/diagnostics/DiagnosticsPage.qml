import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

AppPage {
    id: page
    required property var session
    required property var presentation
    property var screenService: typeof watchScreen === "undefined" ? null : watchScreen
    property int tab: 0
    positionKey: "diagnostics-" + tab
    signal exportRequested
    SectionTabs {
        Layout.fillWidth: true
        labels: Theme.developerMode ? [qsTr("Telemetry"), qsTr("Watch screen"), qsTr("Connection log")] : [qsTr("Telemetry"), qsTr("Watch screen")]
        currentIndex: page.tab
        onCurrentIndexChanged: page.tab = currentIndex
    }
    Connections { target: Theme; function onDeveloperModeChanged() { if (!Theme.developerMode && page.tab > 1) page.tab = 0 } }
    StatusCard { visible: page.tab === 0; session: page.session; presentation: page.presentation; onExportRequested: page.exportRequested() }
    AppCard {
        visible: page.tab === 0
        Layout.fillWidth: true
        implicitHeight: connectionState.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: connectionState; anchors.fill: parent; spacing: 10
            Label { text: page.session.connectionState + " · " + page.session.transport; textFormat: Text.PlainText; font.weight: Font.DemiBold; Layout.fillWidth: true; wrapMode: Text.WordWrap }
            Caption { text: page.session.error || qsTr("No connection errors"); color: page.session.error ? Theme.error : Theme.muted; Layout.fillWidth: true }
        }
    }
    WatchPreview {
        visible: page.tab === 1
        frameVisible: page.tab === 1 && (!page.inSwipeView || page.currentSwipePage)
        session: page.session
        Layout.fillWidth: true
        Layout.preferredHeight: 440
    }
    AppCard {
        visible: page.tab === 1
        Layout.fillWidth: true
        implicitHeight: controls.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: controls; anchors.fill: parent; spacing: 14
            Label { text: qsTr("Watch controls"); font.pixelSize: 18; font.weight: Font.DemiBold; Layout.fillWidth: true }
            Flow {
                Layout.fillWidth: true; Layout.preferredHeight: childrenRect.height; spacing: 8
                Repeater {
                    model: [{key:"previous",label:qsTr("Previous"),icon:"left"},{key:"enter",label:qsTr("Enter"),icon:"check"},{key:"next",label:qsTr("Next"),icon:"right"},{key:"back",label:qsTr("Back"),icon:"left"},{key:"settings",label:qsTr("Settings"),icon:"settings"},{key:"plus",label:"+",icon:""},{key:"minus",label:"−",icon:""}]
                    delegate: AppButton {
                        required property var modelData
                        iconName: modelData.icon
                        text: modelData.label
                        available: page.session.connected && page.session.transport === "USB" && !page.session.interactionBusy
                        onClicked: { if (page.screenService && page.screenService.supported) page.screenService.sendKey(modelData.key); else page.session.request("KEY",modelData.key) }
                    }
                }
            }
            Caption { text: qsTr("USB input replaces physical buttons on this board."); Layout.fillWidth: true }
        }
    }
    UsbLogCard { visible: Theme.developerMode && page.tab === 2; session: page.session; Layout.fillWidth: true }
}
