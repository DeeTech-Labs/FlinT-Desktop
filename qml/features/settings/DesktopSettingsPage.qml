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
        implicitHeight: appearance.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: appearance; anchors.fill: parent; spacing: 14
            Label { text: qsTr("Appearance"); font.pixelSize: 21; font.weight: Font.DemiBold; Layout.fillWidth: true }
            GridLayout {
                Layout.fillWidth: true; columns: width < 580 ? 1 : 2; columnSpacing: 24; rowSpacing: 16
                ColumnLayout {
                    Layout.fillWidth: true; Layout.preferredWidth: 1
                    Caption { text: qsTr("Color theme"); Layout.fillWidth: true }
                    AppComboBox {
                        objectName: "themeSelector"
                        Layout.fillWidth: true
                        model: [qsTr("System theme"), qsTr("Light theme"), qsTr("Dark theme")]
                        currentIndex: Theme.mode
                        Accessible.name: qsTr("Color theme")
                        onActivated: Theme.mode = currentIndex
                    }
                }
                ColumnLayout {
                    Layout.fillWidth: true; Layout.preferredWidth: 1
                    Caption { text: qsTr("Interface language"); Layout.fillWidth: true }
                    AppComboBox {
                        id: languageSelector
                        objectName: "languageSelector"
                        Layout.fillWidth: true
                        model: localization.languages
                        textRole: "name"; valueRole: "code"
                        Accessible.name: qsTr("Interface language")
                        currentIndex: indexOfValue(localization.language)
                        onActivated: localization.language = currentValue
                    }
                }
            }
            AppSwitch { objectName: "animationToggle"; Layout.fillWidth: true; text: qsTr("Interface animations"); checked: Theme.animations; onToggled: Theme.animations = checked }
        }
    }
    AppCard {
        Layout.fillWidth: true
        implicitHeight: behavior.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: behavior; anchors.fill: parent; spacing: 14
            Label { text: qsTr("Connection and background"); font.pixelSize: 21; font.weight: Font.DemiBold; Layout.fillWidth: true; wrapMode: Text.WordWrap }
            AppSwitch { Layout.fillWidth: true; text: qsTr("Synchronize time automatically on connection"); checked: page.session.autoSyncTime; onToggled: page.session.autoSyncTime = checked }
            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }
            AppSwitch { Layout.fillWidth: true; text: qsTr("Keep running in the system tray"); enabled: page.backgroundService !== null && page.backgroundService.available; checked: page.backgroundService !== null && page.backgroundService.enabled; onToggled: if (page.backgroundService) page.backgroundService.enabled = checked }
            Caption { text: qsTr("With tray mode enabled, closing the window keeps connections and companion tools running. Quit from the tray to stop."); Layout.fillWidth: true }
        }
    }
    AppCard {
        Layout.fillWidth: true
        implicitHeight: advanced.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: advanced; anchors.fill: parent; spacing: 14
            Label { text: qsTr("Developer tools"); font.pixelSize: 21; font.weight: Font.DemiBold; Layout.fillWidth: true }
            AppSwitch { Layout.fillWidth: true; text: qsTr("Developer mode"); checked: Theme.developerMode; onToggled: Theme.developerMode = checked }
            Caption { text: qsTr("Try the interface without hardware. Simulator data stays separate from real device captures."); Layout.fillWidth: true }
            AppButton { text: qsTr("Start simulator"); iconName: "play"; available: !page.session.connected && !page.session.connecting; onClicked: page.session.startSimulator() }
        }
    }
}
