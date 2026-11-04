import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Flare

ApplicationWindow {
    id: app
    width: 1320
    height: 880
    minimumWidth: 540
    minimumHeight: 480
    visible: true
    title: Qt.application.name
    color: Theme.background
    font.family: Theme.fontFamily
    font.pixelSize: 14
    readonly property bool wantsPreview: visible && visibility !== Window.Minimized && (section === 0 || section === 1 || section === 4)
    onWantsPreviewChanged: preview.active = wantsPreview
    Component.onCompleted: preview.active = wantsPreview
    property int section: 0
    onClosing: close => {
        if (backgroundAgent.enabled) {
            close.accepted = false;
            hide();
        }
    }
    readonly property bool compact: width < Theme.compactWidth
    property var history: [0]
    property int historyPosition: 0
    property bool restoringHistory: false
    onSectionChanged: {
        if (!restoringHistory && history[historyPosition] !== section) {
            const next = history.slice(0, historyPosition + 1);
            next.push(section);
            history = next;
            historyPosition = next.length - 1;
        }
    }
    function goBack() {
        if (historyPosition > 0) {
            restoringHistory = true;
            historyPosition--;
            section = history[historyPosition];
            restoringHistory = false;
        }
    }
    function goForward() {
        if (historyPosition + 1 < history.length) {
            restoringHistory = true;
            historyPosition++;
            section = history[historyPosition];
            restoringHistory = false;
        }
    }
    Shortcut {
        sequences: [StandardKey.Back]
        onActivated: app.goBack()
    }
    Shortcut {
        sequences: [StandardKey.Forward]
        onActivated: app.goForward()
    }
    Shortcut {
        sequence: "Alt+1"
        onActivated: app.section = 0
    }
    Shortcut {
        sequence: "Alt+2"
        onActivated: app.section = 1
    }
    Shortcut {
        sequence: "Alt+3"
        onActivated: app.section = 2
    }
    Shortcut {
        sequence: "Alt+4"
        onActivated: app.section = 3
    }
    palette.window: Theme.surface
    palette.windowText: Theme.text
    palette.text: Theme.text
    palette.base: Theme.base
    palette.button: Theme.button
    palette.buttonText: Theme.text
    palette.highlight: Theme.accent
    palette.highlightedText: Theme.accentText
    DevicePresentation {
        id: presentation
        session: client
    }
    GridLayout {
        anchors.fill: parent
        anchors.margins: 8
        columns: app.compact ? 1 : 2
        columnSpacing: 0
        rowSpacing: 0
        AppNavigation {
            session: client
            compact: app.compact
            currentIndex: app.section
            onSelected: index => app.section = index
            Layout.fillWidth: app.compact
            Layout.fillHeight: !app.compact
            Layout.preferredWidth: app.compact ? -1 : 60
        }
        Rectangle {
            color: Theme.base
            radius: 18
            Layout.fillWidth: true
            Layout.fillHeight: true
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: app.compact ? 14 : Theme.pageMargin
                spacing: Theme.spacing
                RowLayout {
                    Layout.fillWidth: true
                    AppButton {
                        iconName: "left"
                        text: ""
                        available: app.historyPosition > 0
                        onClicked: app.goBack()
                    }
                    Label {
                        text: [qsTr("Overview"), qsTr("My watch"), "Ignite Hub", "Companion", qsTr("Diagnostics"), qsTr("Flare settings"), qsTr("Connect your watch")][app.section]
                        font.pixelSize: app.compact ? 24 : 28
                        font.weight: Font.DemiBold
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }
                    AppButton {
                        iconName: client.transport === "BLE" ? "bluetooth" : "connection"
                        text: app.compact ? "" : client.connected ? client.transport : qsTr("Connect")
                        onClicked: app.section = 6
                    }
                }
                SwipeView {
                    id: pages
                    objectName: "pageStack"
                    clip: true
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: app.section
                    onCurrentIndexChanged: if (app.section !== currentIndex)
                        app.section = currentIndex
                    OverviewPage {
                        session: client
                        presentation: presentation
                        onConnectRequested: app.section = 6
                        onWatchRequested: app.section = 1
                        onFacesRequested: app.section = 2
                        onCompanionRequested: app.section = 3
                    }
                    WatchSettingsPage {
                        session: client
                    }
                    FaceGalleryPage {}
                    CompanionPage {}
                    DiagnosticsPage {
                        session: client
                        presentation: presentation
                    }
                    DesktopSettingsPage {
                        session: client
                        backgroundService: backgroundAgent
                    }
                    AppPage {
                        ConnectionCard {
                            session: client
                            Layout.fillWidth: true
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    AppIcon {
                        name: "shield"
                        width: 12
                        height: 12
                        tint: Theme.muted
                    }
                    Caption {
                        text: qsTr("Data stays on this computer and your watch.")
                        font.pixelSize: 10
                        Layout.fillWidth: true
                    }
                    Caption {
                        text: "Flint Flare"
                        font.pixelSize: 10
                    }
                }
            }
        }
    }
    Popup {
        id: toast
        parent: Overlay.overlay
        x: Math.max(12, (app.width - width) / 2)
        y: app.height - height - 24
        width: Math.min(app.width - 32, 620)
        padding: 18
        background: Rectangle {
            color: Theme.surface
            radius: 14
            border.color: Theme.inputBorder
        }
        contentItem: Label {
            id: toastText
            color: Theme.error
            wrapMode: Text.WordWrap
            textFormat: Text.PlainText
        }
        Timer {
            id: toastTimer
            interval: 6500
            onTriggered: toast.close()
        }
    }
    Connections {
        target: client
        function onFailure(message) {
            toastText.text = message;
            toast.open();
            toastTimer.restart();
        }
    }
    Popup {
        visible: client.pairingCode.length > 0
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        closePolicy: Popup.NoAutoClose
        width: 440
        padding: 20
        background: Rectangle {
            color: Theme.surface
            radius: 16
            border.color: Theme.inputBorder
        }
        contentItem: ColumnLayout {
            Label {
                text: qsTr("Confirm pairing")
                font.pixelSize: 22
            }
            Label {
                text: client.pairingCode
                font.pixelSize: 36
            }
            Caption {
                text: qsTr("Confirm the same code over the local USB connection.")
                Layout.fillWidth: true
            }
            AppButton {
                text: qsTr("Codes match")
                primary: true
                onClicked: client.confirmPairing(true)
            }
            AppButton {
                text: qsTr("Reject")
                onClicked: client.confirmPairing(false)
            }
        }
    }
}
