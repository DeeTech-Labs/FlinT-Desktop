import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

Button {
    id: tile
    property string iconName: "grid"
    property string detail: ""
    implicitHeight: Math.max(76, contentItem.implicitHeight + 24)
    implicitWidth: 220
    padding: 12
    hoverEnabled: true
    contentItem: RowLayout {
        spacing: 10
        Rectangle {
            Layout.preferredWidth: 32; Layout.preferredHeight: 36
            radius: 9; color: Theme.metric
            AppIcon { anchors.centerIn: parent; name: tile.iconName; width: 20; height: 20; tint: Theme.accent }
        }
        ColumnLayout {
            Layout.fillWidth: true; Layout.minimumWidth: 0
            spacing: 6
            Text { font.family: Theme.fontFamily; text: tile.text; color: Theme.text; font.pixelSize: 13; font.weight: Font.DemiBold; Layout.fillWidth: true; elide: Text.ElideRight }
            Text { font.family: Theme.fontFamily; text: tile.detail; color: Theme.muted; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        }
        AppIcon { name: "right"; tint: tile.hovered ? Theme.accent : Theme.muted; Layout.preferredWidth: 14; Layout.preferredHeight: 14 }
    }
    background: Rectangle {
        radius: 12
        color: tile.down ? Theme.metric : tile.hovered ? Theme.buttonHover : Theme.surface
        border.color: tile.visualFocus ? Theme.accent : "transparent"
        Behavior on color { ColorAnimation { duration: Theme.animationDuration } }
    }
}
