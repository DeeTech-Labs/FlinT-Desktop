import QtQuick
import QtQuick.Controls.Basic
import Flint.Desktop

Switch {
    id: toggle
    implicitHeight: Math.max(36,contentItem.implicitHeight + 12)
    spacing: 14
    padding: 0
    hoverEnabled: true
    contentItem: Text {
        text: toggle.text
        color: toggle.enabled ? Theme.text : Theme.muted
        font.pixelSize: 13
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.WordWrap
        rightPadding: toggle.indicator.width + toggle.spacing
    }
    indicator: Rectangle {
        width: 36; height: 20; radius: 10
        x: toggle.width - width
        y: (toggle.height - height) / 2
        color: toggle.checked ? Theme.accent : Theme.buttonHover
        border.color: toggle.activeFocus ? Theme.accent : toggle.checked ? Theme.accent : Theme.inputBorder
        Rectangle { width: 14; height: 14; radius: 7; y: 3; x: toggle.checked ? 19 : 3; color: toggle.checked ? "#ffffff" : Theme.muted; Behavior on x { NumberAnimation { duration: Theme.animationDuration } } }
    }
}
