import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

AppPage {
    id: page
    required property var session
    property var backgroundService: null
    AppCard {
        Layout.fillWidth: true
        implicitHeight: settingsContent.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: settingsContent; anchors.fill: parent; spacing: 16
            Label { text: qsTr("Desktop settings"); font.pixelSize: 24; font.bold: true; Layout.fillWidth: true }
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
            AppSwitch { Layout.fillWidth: true; Layout.minimumWidth: 0; Component.onCompleted: contentItem.wrapMode = Text.WordWrap; text: qsTr("Keep running in the system tray"); enabled: page.backgroundService !== null && page.backgroundService.available; checked: page.backgroundService !== null && page.backgroundService.enabled; onToggled: page.backgroundService.enabled = checked }
            Caption { text: qsTr("With tray mode enabled, closing the window keeps connections and companion tools running. Quit from the tray to stop."); Layout.fillWidth: true }
            AppSwitch { Layout.fillWidth: true; Layout.minimumWidth: 0; Component.onCompleted: contentItem.wrapMode = Text.WordWrap; text: qsTr("Synchronize time automatically on connection"); checked: page.session.autoSyncTime; onToggled: page.session.autoSyncTime = checked }
            AppSwitch { Layout.fillWidth: true; Layout.minimumWidth: 0; Component.onCompleted: contentItem.wrapMode = Text.WordWrap; text: qsTr("Developer mode"); checked: Theme.developerMode; onToggled: Theme.developerMode = checked }
            AppSwitch { Layout.fillWidth: true; Layout.minimumWidth: 0; Component.onCompleted: contentItem.wrapMode = Text.WordWrap; text: qsTr("Interface animations"); checked: Theme.animations; onToggled: Theme.animations = checked }
            Caption { text: qsTr("Try the interface without hardware. Simulator data stays separate from real device captures."); Layout.fillWidth: true }
            AppButton { text: qsTr("Start simulator"); onClicked: page.session.startSimulator() }
        }
    }
}
