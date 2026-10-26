import QtQuick
import QtQuick.Controls.Basic
import Flint.Desktop

TextField {
    property bool blockPageGestures: true
    id: input
    implicitHeight: Theme.controlHeight
    leftPadding: 13
    rightPadding: 13
    font.pixelSize: 13
    color: Theme.text
    placeholderTextColor: Theme.muted
    selectionColor: Theme.accent
    selectedTextColor: "white"
    background: Rectangle { Behavior on border.color { ColorAnimation { duration: Theme.animationDuration } } radius: 11; color: Theme.input; border.color: input.activeFocus ? Theme.accent : Theme.inputBorder }
}
