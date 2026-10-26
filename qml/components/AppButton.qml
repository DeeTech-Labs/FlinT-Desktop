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
    Accessible.name: text
    activeFocusOnTab: true
    implicitHeight: Theme.controlHeight
    implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
    leftPadding: text.length ? 12 : 10
    rightPadding: text.length ? 12 : 10
    hoverEnabled: true
    contentItem: Row {
        spacing: action.iconName ? 9 : 0
        opacity: action.enabled ? 1 : 0.45
        AppIcon { visible: action.iconName !== ""; name: action.iconName; width: visible ? 16 : 0; height: 16; anchors.verticalCenter: parent.verticalCenter; tint: action.primary ? Theme.primaryText : Theme.buttonText }
        Text { font.family: Theme.fontFamily;
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
        radius: 9
        Behavior on color { ColorAnimation { duration: Theme.animationDuration } }
        Behavior on border.color { ColorAnimation { duration: Theme.animationDuration } }
        border.width: 1
        border.color: action.visualFocus ? Theme.accent : action.primary ? Theme.accent : "transparent"
        color: action.primary ? (action.down ? Theme.primaryPressed : action.hovered ? Theme.primaryHover : Theme.accent) : (action.down ? Theme.navSelected : action.hovered ? Theme.buttonHover : Theme.button)
        opacity: action.enabled ? 1 : 0.6
    }
}
