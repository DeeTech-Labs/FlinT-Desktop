import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

ColumnLayout {
    id: control
    required property var service
    required property string setting
    required property string label
    required property var choices
    Layout.fillWidth: true
    spacing: 6
    Caption { text: control.label; Layout.fillWidth: true }
    AppComboBox {
        Layout.fillWidth: true
        Accessible.name: control.label
        model: control.choices
        textRole: "label"
        valueRole: "value"
        enabled: control.service.supported
        currentIndex: indexOfValue(control.service.draft[control.setting])
        onActivated: control.service.edit(control.setting, currentValue)
    }
}
