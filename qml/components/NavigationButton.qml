import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

Button {
    id: control
    property bool selected: false
    property bool compact: false
    property string iconName: "home"
    implicitHeight: 38
    padding: 7
    hoverEnabled: true
    Accessible.role: Accessible.PageTab
    Accessible.name: text
    ToolTip.visible: hovered && compact
    ToolTip.text: text
    contentItem: ColumnLayout {
        spacing: 4
        AppIcon {
            name: control.iconName
            tint: control.selected ? (Theme.light ? Theme.accent : "#09293b") : control.hovered ? Theme.text : Theme.navText
            Layout.preferredWidth: 20; Layout.preferredHeight: 20
            Layout.alignment: Qt.AlignHCenter
        }
        Text { font.family: Theme.fontFamily;
            visible: !control.compact
            text: control.text
            font.pixelSize: 10
            font.weight: control.selected ? Font.DemiBold : Font.Medium
            color: control.selected ? (Theme.light ? Theme.accent : "#09293b") : Theme.navText
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
        }
    }
    background: Rectangle {
        radius: 10
        color: control.selected ? Theme.navSelected : control.hovered ? Theme.navHover : "transparent"
        border.color: control.visualFocus ? Theme.accent : "transparent"
        Behavior on color { ColorAnimation { duration: Theme.animationDuration } }
    }
}
