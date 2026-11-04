import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

AppCard {
    required property var session
    implicitHeight: 220
    ColumnLayout {
        anchors.fill: parent
        spacing: 12
        Label {
            text: qsTr("Connection status")
            font.pixelSize: 20
            font.weight: Font.DemiBold
        }
        LogView {
            text: session.status + "\n" + (session.info.build || "")
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
