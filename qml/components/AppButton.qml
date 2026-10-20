import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

Button {
    id: action
    property bool primary: false
    property string iconName: ""
    property bool available: true
    property bool busy: false
    enabled: available && !busy
    implicitHeight: Theme.controlHeight
    implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
    leftPadding: 16
    rightPadding: 16
    hoverEnabled: true
    contentItem: Row {
        spacing: action.iconName ? 9 : 0
        opacity: action.available ? 1 : 0.45
        AppIcon { visible: action.iconName !== ""; name: action.iconName; width: visible ? 18 : 0; height: 18; anchors.verticalCenter: parent.verticalCenter; tint: action.primary ? Theme.primaryText : Theme.buttonText }
        Text {
            text: action.text
            textFormat: Text.PlainText
            font.pixelSize: 13
            font.weight: Font.DemiBold
            color: action.primary ? Theme.primaryText : Theme.buttonText
            height: parent.height
            verticalAlignment: Text.AlignVCenter
        }
    }
    background: Rectangle {
        radius: 10
        border.width: 1
        border.color: action.visualFocus ? Theme.accent : action.primary ? "#568dff" : Theme.divider
        color: action.primary ? (action.down ? Theme.primaryPressed : action.hovered ? Theme.primaryHover : Theme.accent) : (action.down ? Theme.navSelected : action.hovered ? Theme.buttonHover : Theme.button)
        opacity: action.available ? 1 : 0.6
    }
}
