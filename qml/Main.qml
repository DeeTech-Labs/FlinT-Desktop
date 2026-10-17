import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window

    width: 1100
    height: 720
    minimumWidth: 640
    minimumHeight: 480
    visible: true
    title: Qt.application.name

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 64, 480)
        spacing: 12

        Label {
            Layout.fillWidth: true
            text: Qt.application.name
            font.pixelSize: 28
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Spark и другие устройства на FlinT OS")
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
