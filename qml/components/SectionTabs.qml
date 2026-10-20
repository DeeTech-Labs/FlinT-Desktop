import QtQuick
import QtQuick.Controls.Basic
import Flint.Desktop

TabBar {
    id: tabs
    property var labels: []
    implicitWidth: 0
    implicitHeight: 47
    spacing: 5
    padding: 4
    background: Rectangle { radius: 12; color: Theme.base; border.color: Theme.divider }
    Repeater {
        model: tabs.labels
        delegate: TabButton {
            required property string modelData
            required property int index
            width: (tabs.availableWidth - Math.max(0,tabs.labels.length - 1) * tabs.spacing) / Math.max(1,tabs.labels.length)
            height: tabs.availableHeight
            hoverEnabled: true
            contentItem: Text { text: modelData; color: checked ? Theme.text : Theme.muted; font.pixelSize: 12; font.weight: checked ? Font.DemiBold : Font.Medium; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight }
            background: Rectangle { radius: 8; color: checked ? Theme.navSelected : parent.hovered ? Theme.navHover : "transparent"; border.color: checked ? (Theme.light ? "#bbd1ff" : "#315595") : "transparent"; Rectangle { visible: checked; height: 2; width: parent.width - 20; anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter; color: Theme.accent; radius: 1 } }
        }
    }
}
