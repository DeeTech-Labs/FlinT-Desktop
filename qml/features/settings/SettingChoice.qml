import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Flare

ColumnLayout {
    id: control
    required property string label
    required property var choices
    property alias currentIndex: selector.currentIndex
    signal selected(var value)
    Layout.fillWidth: true
    Caption {
        text: control.label
        Layout.fillWidth: true
    }
    AppComboBox {
        id: selector
        Layout.fillWidth: true
        model: control.choices
        textRole: "label"
        valueRole: "value"
        onActivated: control.selected(currentValue)
    }
}
