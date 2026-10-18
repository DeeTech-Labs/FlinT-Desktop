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
            CheckBox { text: qsTr("Keep running in the system tray"); enabled: page.backgroundService !== null && page.backgroundService.available; checked: page.backgroundService !== null && page.backgroundService.enabled; onToggled: page.backgroundService.enabled = checked }
            Caption { text: qsTr("With tray mode enabled, closing the window keeps connections and companion tools running. Quit from the tray to stop."); Layout.fillWidth: true }
            CheckBox { text: qsTr("Synchronize time automatically on connection"); checked: page.session.autoSyncTime; onToggled: page.session.autoSyncTime = checked }
            CheckBox { text: qsTr("Developer mode"); checked: Theme.developerMode; onToggled: Theme.developerMode = checked }
            CheckBox { text: qsTr("Interface animations"); checked: Theme.animations; onToggled: Theme.animations = checked }
            Caption { text: qsTr("Try the interface without hardware. Simulator data stays separate from real device captures."); Layout.fillWidth: true }
            AppButton { text: qsTr("Start simulator"); onClicked: page.session.startSimulator() }
        }
    }
}
