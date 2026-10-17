import QtQuick
import QtQuick.Controls.Basic
import Flint.Desktop

ScrollView {
    id: view
    property alias text: logText.text
    property alias placeholderText: logText.placeholderText
    clip: true
    TextArea {
        id: logText
        readOnly: true
        selectByMouse: true
        textFormat: TextEdit.PlainText
        wrapMode: TextEdit.WrapAnywhere
        font.family: Theme.monoFamily
        font.pixelSize: 12
        color: Theme.logText
        padding: 16
        background: Rectangle {
            color: Theme.logBackground
            radius: 12
        }
        onTextChanged: cursorPosition = length
    }
}
