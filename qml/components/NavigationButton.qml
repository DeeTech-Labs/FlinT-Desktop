import QtQuick
import QtQuick.Controls.Basic
import Flint.Desktop

Button {
    id: control
    property bool selected: false
    implicitHeight: 46
    leftPadding: 14
    rightPadding: 14
    hoverEnabled: true
    Accessible.role: Accessible.PageTab
    Accessible.name: text
    contentItem: Text {
        text: control.text
        font.pixelSize: control.font.pixelSize
        font.weight: Font.DemiBold
        color: control.selected ? Theme.accent : Theme.navText
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        radius: 12
        color: control.selected ? Theme.navSelected : control.hovered ? Theme.navHover : Theme.transparent
        border.width: control.activeFocus ? 1 : 0
        border.color: Theme.accent
    }
}
