import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

Rectangle {
    property string label
    property string detail
    implicitHeight: Math.max(110, content.implicitHeight + 32)
    color: Theme.metric
    radius: 14
    ColumnLayout {
        id: content
        anchors.fill: parent
        anchors.margins: 16
        spacing: 8
        Caption {
            text: label
            Layout.fillWidth: true
        }
        Label {
            text: detail
            textFormat: Text.PlainText
            font.pixelSize: 17
            font.weight: Font.DemiBold
            Layout.fillWidth: true
            wrapMode: Text.WrapAnywhere
        }
    }
}
