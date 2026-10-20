import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

Button {
    id: control
    property bool selected: false
    property bool compact: false
    property string iconName: "home"
    implicitHeight: 46
    leftPadding: compact ? 12 : 14
    rightPadding: 12
    hoverEnabled: true
    Accessible.role: Accessible.PageTab
    Accessible.name: text
    ToolTip.visible: hovered && compact
    ToolTip.text: text
    contentItem: RowLayout {
        spacing: 12
        AppIcon { name: control.iconName; tint: control.selected ? (Theme.light ? Theme.accent : "#ffffff") : Theme.navText; Layout.preferredWidth: 20; Layout.preferredHeight: 20; Layout.alignment: Qt.AlignVCenter }
        Text {
            visible: !control.compact
            text: control.text
            font.pixelSize: 13
            font.weight: control.selected ? Font.DemiBold : Font.Medium
            color: control.selected ? (Theme.light ? Theme.accent : "#ffffff") : Theme.navText
            Layout.fillWidth: true
            elide: Text.ElideRight
        }
    }
    background: Rectangle {
        radius: 9
        color: control.selected ? Theme.navSelected : control.hovered ? Theme.navHover : Theme.transparent
        border.width: control.selected || control.activeFocus ? 1 : 0
        border.color: control.selected ? (Theme.light ? "#b4cfff" : "#305da4") : Theme.accent
    }
}
