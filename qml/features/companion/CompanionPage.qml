import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

AppPage {
    AppCard {
        Layout.fillWidth: true
        implicitHeight: content.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: content
            anchors.fill: parent
            spacing: 14
            RowLayout {
                AppIcon {
                    name: "companion"
                    width: 24
                    height: 24
                    tint: Theme.accent
                }
                Label {
                    text: "PC Companion"
                    font.pixelSize: 22
                    font.weight: Font.DemiBold
                }
            }
            Caption {
                text: qsTr("Computer data is shared only when both Desktop and the active Ignite permit it.")
                Layout.fillWidth: true
            }
            AppSwitch {
                Layout.fillWidth: true
                text: qsTr("Allow CPU and RAM metrics")
                checked: companion.metricsAllowed
                enabled: client.connected
                onToggled: companion.metricsAllowed = checked
            }
            Caption {
                text: companion.status
                Layout.fillWidth: true
            }
            Caption {
                text: qsTr("Select the application in Ignite Hub and grant companion.pc.metrics for cpu,ram. Revoking access stops the data flow.")
                Layout.fillWidth: true
            }
        }
    }
}
