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
    Layout.preferredWidth: 1
    Layout.minimumWidth: 0
    spacing: 6
    Caption { text: control.label; Layout.fillWidth: true }
    AppComboBox {
        Layout.fillWidth: true
        objectName: "choice-" + control.setting
        Accessible.name: control.label
        model: control.choices
        textRole: "label"
        valueRole: "value"
        enabled: control.service.supported
        currentIndex: count > 0 ? control.choices.findIndex(choice => Number(choice.value) === Number(control.service.draft[control.setting])) : -1
        onActivated: control.service.edit(control.setting, currentValue)
    }
}
