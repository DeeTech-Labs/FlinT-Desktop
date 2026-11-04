import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

AppCard {
    id: card
    required property var session
    required property var presentation
    implicitHeight: details.implicitHeight + topPadding + bottomPadding
    ColumnLayout {
        id: details
        anchors.fill: parent
        spacing: 14
        RowLayout {
            AppIcon {
                name: "info"
                width: 22
                height: 22
                tint: Theme.accent
            }
            Label {
                text: qsTr("About the device")
                font.pixelSize: 20
                font.weight: Font.DemiBold
            }
        }
        GridLayout {
            Layout.fillWidth: true
            columns: width < 600 ? 1 : 2
            columnSpacing: 16
            rowSpacing: 12
            MetricCard {
                label: qsTr("Model")
                detail: card.session.info.profile || "—"
                Layout.fillWidth: true
            }
            MetricCard {
                label: qsTr("Internal RAM budget")
                detail: (card.session.info.internal || 0) + " B"
                Layout.fillWidth: true
            }
            MetricCard {
                label: "PSRAM"
                detail: (card.session.info.external || 0) + " B"
                Layout.fillWidth: true
            }
            MetricCard {
                label: qsTr("Free storage")
                detail: (card.session.info.storage || 0) + " B"
                Layout.fillWidth: true
            }
        }
        Caption {
            text: qsTr("Build") + ": " + (card.session.info.build || "—")
            Layout.fillWidth: true
        }
    }
}
