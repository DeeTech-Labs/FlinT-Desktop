import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

Rectangle {
    property string label
    property string detail
    implicitHeight: Math.max(96, content.implicitHeight + 32)
    color: Theme.metric
    radius: 14
    ColumnLayout {
        id: content
        anchors.fill: parent
        anchors.margins: 14
        spacing: 8
        Caption {
            text: label
            Layout.fillWidth: true
        }
        Label {
            text: detail
            textFormat: Text.PlainText
            font.pixelSize: 22
            font.weight: Font.Medium
            Layout.fillWidth: true
            wrapMode: Text.WrapAnywhere
        }
    }
}
