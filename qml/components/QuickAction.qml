import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

Button {
    id: tile
    property string iconName: "grid"
    property string detail: ""
    implicitHeight: 132
    implicitWidth: 156
    padding: 18
    hoverEnabled: true
    contentItem: Column {
        spacing: 9
        AppIcon { name: tile.iconName; width: 25; height: 25; tint: tile.hovered ? Theme.accent : "#90b5ff" }
        Text { text: tile.text; color: Theme.text; font.pixelSize: 13; font.weight: Font.DemiBold; width: parent.width; elide: Text.ElideRight }
        Text { text: tile.detail; color: Theme.muted; font.pixelSize: 11; width: parent.width; wrapMode: Text.WordWrap }
    }
    background: Rectangle { radius: 13; color: tile.down ? Theme.navSelected : tile.hovered ? Theme.buttonHover : Theme.surface; border.color: tile.hovered ? "#3b6198" : Theme.divider }
}
