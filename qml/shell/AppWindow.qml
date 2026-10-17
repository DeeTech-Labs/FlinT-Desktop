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
                    text: [qsTr("Everything in sync"), qsTr("Clock data"), qsTr("USB log")][app.section]
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
            AppCard {
                visible: app.session.error.length > 0 || app.session.notification.length > 0
                Layout.fillWidth: true
                implicitHeight: message.implicitHeight + topPadding + bottomPadding
                Label {
                    id: message
                    width: parent.width
                    text: app.session.error.length > 0 ? app.session.error : app.session.notification
                    color: app.session.error.length > 0 ? Theme.error : Theme.accent
                    textFormat: Text.PlainText
                    wrapMode: Text.WordWrap
                }
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
                DevicePage {
                    session: app.session
                    presentation: devicePresentation
                    onExportRequested: exportDialog.open()
                }
                DiagnosticsPage {
                    session: app.session
                }
            }
            Caption {
                text: qsTr("Data is saved to a file only when you request it.")
                font.pixelSize: 10
                Layout.fillWidth: true
            }
        }
    }
}
