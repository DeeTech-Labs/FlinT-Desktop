import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

AppCard {
    id: card
    required property var session
    required property var presentation
    signal exportRequested
    implicitHeight: content.implicitHeight + topPadding + bottomPadding
    Layout.fillWidth: true
    ColumnLayout {
        id: content
        anchors.fill: parent
        spacing: 20
        RowLayout {
            AppIcon { name: "info"; tint: "#83acff"; Layout.preferredWidth: 22; Layout.preferredHeight: 22 }
            Label { text: qsTr("About the device"); font.pixelSize: 20; font.weight: Font.DemiBold; Layout.fillWidth: true }
        }
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 16
            rowSpacing: 18
            Caption { text: qsTr("Model") }
            Label { text: card.presentation.value(card.session.info,"model"); textFormat: Text.PlainText; font.pixelSize: 13; Layout.fillWidth: true; elide: Text.ElideRight }
            Caption { text: qsTr("Firmware") }
            Label { text: card.presentation.value(card.session.info,"firmware"); font.pixelSize: 13; Layout.fillWidth: true }
            Caption { text: qsTr("Chip") }
            Label { text: card.presentation.value(card.session.info,"chip"); font.pixelSize: 13; Layout.fillWidth: true }
            Caption { text: qsTr("Device ID") }
            Label { text: card.presentation.value(card.session.info,"id"); font.pixelSize: 12; font.family: Theme.monoFamily; Layout.fillWidth: true; elide: Text.ElideRight }
            Caption { text: qsTr("Screen") }
            Label { text: card.session.info.screen ? card.session.info.screen.width + " × " + card.session.info.screen.height : "—"; font.pixelSize: 13; Layout.fillWidth: true }
            Caption { text: qsTr("Free memory") }
            Label { text: card.session.status.free_heap_bytes !== undefined ? Math.round(card.session.status.free_heap_bytes / 1024) + " KB" : "—"; font.pixelSize: 13; Layout.fillWidth: true }
        }
    }
}
