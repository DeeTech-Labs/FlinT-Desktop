import QtQuick
import QtQuick.Controls.Basic
import Flint.Desktop

TabBar {
    id: tabs
    property var labels: []
    implicitWidth: 0
    implicitHeight: 40
    spacing: 0
    padding: 0
    background: Rectangle { color: "transparent"; Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.divider } }
    Repeater {
        model: tabs.labels
        delegate: TabButton {
            required property string modelData
            required property int index
            width: tabs.availableWidth / Math.max(1, tabs.labels.length)
            height: tabs.availableHeight
            hoverEnabled: true
            ToolTip.visible: hovered && contentItem.truncated
            ToolTip.text: modelData
            contentItem: Text { font.family: Theme.fontFamily;
                text: modelData; color: checked ? Theme.text : Theme.muted
                font.pixelSize: 12; font.weight: checked ? Font.DemiBold : Font.Normal
                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight
            }
            background: Rectangle {
                color: parent.hovered ? Theme.metric : "transparent"
                radius: 10
                Rectangle {
                    anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width * 0.66; height: 2; radius: 1
                    color: Theme.accent; visible: checked
                }
            }
        }
    }
}
