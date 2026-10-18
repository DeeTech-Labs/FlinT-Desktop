import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Flint.Desktop

ApplicationWindow {
    id: app
    width: 1240
    height: 860
    minimumWidth: 540
    minimumHeight: 480
    visible: true
    title: Qt.application.name
    color: Theme.background
    font.family: Theme.fontFamily
    font.pixelSize: 14
    property var session: device
    property var settingsService: watchSettings
    property var studioService: faceStudio
    property var toolsService: companionTools
    property var backgroundService: typeof backgroundAgent === "undefined" ? null : backgroundAgent
    onClosing: close => { if (backgroundService && backgroundService.enabled) { close.accepted = false; hide() } }
    property int section: 0
    readonly property bool compact: width < Theme.compactWidth
    palette.window: Theme.surface
    palette.mid: Theme.inputBorder
    palette.dark: Theme.muted
    palette.placeholderText: Theme.muted
    palette.windowText: Theme.text
    palette.text: Theme.text
    palette.base: Theme.base
    palette.button: Theme.button
    palette.buttonText: Theme.text
    palette.highlight: Theme.accent
    palette.highlightedText: Theme.accentText

    DevicePresentation {
        id: devicePresentation
        session: app.session
    }
    FileDialog {
        id: exportDialog
        title: qsTr("Save captured data")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("JSON data (*.json)")]
        defaultSuffix: "json"
        onAccepted: app.session.saveData(selectedFile)
    }
    GridLayout {
        anchors.fill: parent
        columns: app.compact ? 1 : 2
        columnSpacing: 0
        rowSpacing: 0
        AppNavigation {
            session: app.session
            compact: app.compact
            currentIndex: app.section
            onSelected: index => app.section = index
            Layout.fillWidth: app.compact
            Layout.fillHeight: !app.compact
            Layout.preferredWidth: app.compact ? -1 : 218
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: app.compact ? 16 : Theme.pageMargin
            spacing: Theme.spacing
            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: [qsTr("Everything in sync"), qsTr("My watch"), qsTr("Watch faces"), "PC Companion", qsTr("Diagnostics"), qsTr("Settings")][app.section]
                    font.pixelSize: app.compact ? 24 : 32
                    font.bold: true
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                BusyIndicator {
                    running: app.session.busy && !app.session.polling
                    implicitWidth: 26
                    implicitHeight: 26
                }
                AppComboBox {
                    objectName: "themeSelector"
                    Layout.preferredWidth: 135
                    model: [qsTr("System theme"), qsTr("Light theme"), qsTr("Dark theme")]
                    currentIndex: Theme.mode
                    Accessible.name: qsTr("Color theme")
                    onActivated: Theme.mode = currentIndex
                }
                AppComboBox {
                    id: languageSelector
                    objectName: "languageSelector"
                    Layout.preferredWidth: 120
                    model: localization.languages
                    textRole: "name"
                    valueRole: "code"
                    Accessible.name: qsTr("Interface language")
                    currentIndex: indexOfValue(localization.language)
                    onActivated: localization.language = currentValue
                }
            }
            Caption {
                text: app.session.connectionState
                Layout.fillWidth: true
            }
            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: app.section
                OverviewPage {
                    session: app.session
                    presentation: devicePresentation
                    onExportRequested: exportDialog.open()
                }
                WatchSettingsPage { service: app.settingsService }
                FaceGalleryPage { service: app.settingsService; studioService: app.studioService }
                CompanionPage { service: app.toolsService }
                AppPage {
                    StatusCard { session: app.session; presentation: devicePresentation; onExportRequested: exportDialog.open() }
                    DeviceInfoCard { session: app.session; presentation: devicePresentation }
                    UsbLogCard { visible: Theme.developerMode; session: app.session }
                }
                DesktopSettingsPage { session: app.session; backgroundService: app.backgroundService }
            }
            Caption {
                text: qsTr("Data stays on this computer and your watch.")
                font.pixelSize: 10
                Layout.fillWidth: true
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
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle { color: Theme.surface; radius: 14; border.color: Theme.inputBorder }
        contentItem: Label {
            text: app.session.error.length ? app.session.error : app.session.notification
            color: app.session.error.length ? Theme.error : Theme.text
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
        }
        enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.animationDuration } }
        exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.animationDuration } }
        Timer { id: toastTimer; interval: 6500; onTriggered: toast.close() }
    }
    Connections {
        target: app.session
        property string previousMessage: ""
        function onStateChanged() {
            const message = app.session.error || app.session.notification
            if (message && message !== previousMessage) { toast.open(); toastTimer.restart() }
            previousMessage = message
        }
    }
}
