import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

Button {
    id: action
    property bool primary: false
    // Preserve appearance during short background requests while blocking duplicate commands.
    property bool available: true
    property bool busy: false
    enabled: available && !busy
    implicitHeight: Theme.controlHeight
    leftPadding: 18
    rightPadding: 18
    hoverEnabled: true
    contentItem: Text {
        text: action.text
        textFormat: Text.PlainText
        font.pixelSize: 13
        font.weight: Font.DemiBold
        color: action.primary ? Theme.primaryText : Theme.buttonText
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        opacity: action.available ? 1 : 0.35
    }
    background: Rectangle {
        border.width: action.visualFocus ? 2 : 0
        border.color: Theme.text
        radius: 21
        color: action.primary ? (action.down ? Theme.primaryPressed : action.hovered ? Theme.primaryHover : Theme.accent) : (action.hovered ? Theme.buttonHover : Theme.button)
        opacity: action.available ? 1 : 0.45
        Behavior on color {
            ColorAnimation {
                duration: Theme.animationDuration
            }
        }
    }
}
