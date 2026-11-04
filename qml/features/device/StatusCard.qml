import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

AppCard {
    required property var session
    implicitHeight: details.implicitHeight + topPadding + bottomPadding
    ColumnLayout {
        id: details
        anchors.fill: parent
        Label {
            text: session.status
            font.pixelSize: 20
        }
        Caption {
            text: qsTr("Active application") + ": " + (session.info.active || "—")
            Layout.fillWidth: true
        }
        AppButton {
            text: qsTr("Refresh")
            iconName: "sync"
            available: session.connected
            onClicked: session.refreshInfo()
        }
    }
}
