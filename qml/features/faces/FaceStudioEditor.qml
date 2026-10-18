import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Flint.Desktop

AppCard {
    id: editor
    required property var service
    Layout.fillWidth: true
    implicitHeight: content.implicitHeight + topPadding + bottomPadding
    FileDialog { id: importFile; fileMode: FileDialog.OpenFile; nameFilters: ["FlinT face (*.flintface *.json)"]; onAccepted: editor.service.importDesign(selectedFile) }
    FileDialog { id: exportFile; fileMode: FileDialog.SaveFile; nameFilters: ["FlinT face (*.flintface)"]; defaultSuffix: "flintface"; onAccepted: editor.service.exportDesign(selectedFile) }
    ColorDialog { id: backgroundPicker; selectedColor: editor.service.design.background; onAccepted: editor.service.edit("background",selectedColor.toString()) }
    ColorDialog { id: foregroundPicker; selectedColor: editor.service.design.foreground; onAccepted: editor.service.edit("foreground",selectedColor.toString()) }
    ColumnLayout {
        id: content; anchors.fill: parent; spacing: 14
        Label { text: "Face Studio / Pixel Lab"; font.pixelSize: 24; font.bold: true; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        Caption { text: qsTr("Create a clock layout with a 16 × 16 pixel icon. Left button draws, right button erases. Preview uses sample time."); Layout.fillWidth: true }
        AppComboBox { id: designs; model: editor.service.library.map(entry => (entry.favorite ? "★ " : "") + entry.design.name); Layout.fillWidth: true; onActivated: editor.service.load(currentIndex) }
        TextField { text: editor.service.design.name; maximumLength: 64; Layout.fillWidth: true; Accessible.name: qsTr("Design name"); onEditingFinished: editor.service.edit("name",text) }
        GridLayout {
            columns: width < 640 ? 1 : 2
            Layout.fillWidth: true
            columnSpacing: 18; rowSpacing: 14
            Rectangle {
                Layout.preferredWidth: 256; Layout.preferredHeight: 256
                Layout.alignment: Qt.AlignHCenter
                color: editor.service.design.background
                Grid {
                    anchors.fill: parent; columns: 16
                    Repeater {
                        model: 256
                        delegate: Rectangle {
                            required property int index
                            width: 16; height: 16
                            color: (parseInt(editor.service.design.pixels.charAt(Math.floor(index / 4)),16) & (8 >> (index % 4))) ? editor.service.design.foreground : editor.service.design.background
                            border.width: 1; border.color: Theme.inputBorder
                        }
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    property bool drawing: true
                    function draw(mouse) { editor.service.paint(Math.floor(mouse.y / 16) * 16 + Math.floor(mouse.x / 16),drawing) }
                    onPressed: mouse => { drawing = mouse.button === Qt.LeftButton; draw(mouse) }
                    onPositionChanged: mouse => { if (pressed && mouse.x >= 0 && mouse.x < width && mouse.y >= 0 && mouse.y < height) draw(mouse) }
                }
            }
            Rectangle {
                Layout.preferredWidth: 280; Layout.preferredHeight: 240
                Layout.alignment: Qt.AlignHCenter
                color: editor.service.design.background
                border.color: Theme.inputBorder; radius: 12
                Grid {
                    x: 116; y: 16; columns: 16
                    Repeater {
                        model: 256
                        delegate: Rectangle {
                            required property int index
                            width: 3; height: 3
                            color: (parseInt(editor.service.design.pixels.charAt(Math.floor(index / 4)),16) & (8 >> (index % 4))) ? editor.service.design.foreground : editor.service.design.background
                        }
                    }
                }
                Text { text: "12:34"; anchors.horizontalCenter: parent.horizontalCenter; y: editor.service.design.timeY - height; font.pixelSize: 46; font.bold: true; color: editor.service.design.foreground }
                Text { visible: editor.service.design.showDate; text: qsTr("17 October"); anchors.horizontalCenter: parent.horizontalCenter; y: editor.service.design.dateY - height; font.pixelSize: 18; color: editor.service.design.foreground }
            }
        }
        Flow {
            Layout.fillWidth: true; Layout.preferredHeight: childrenRect.height; spacing: 10
            AppButton { text: qsTr("Background"); onClicked: backgroundPicker.open() }
            AppButton { text: qsTr("Foreground"); onClicked: foregroundPicker.open() }
            AppButton { text: qsTr("Favorite"); onClicked: editor.service.favorite(designs.currentIndex) }
        }
        Caption { text: qsTr("Time position"); Layout.fillWidth: true }
        Slider { from: 80; to: 150; stepSize: 1; value: editor.service.design.timeY; Layout.fillWidth: true; onMoved: editor.service.edit("timeY",Math.round(value)) }
        Caption { text: qsTr("Date position"); Layout.fillWidth: true }
        Slider { from: 170; to: 220; stepSize: 1; value: editor.service.design.dateY; Layout.fillWidth: true; onMoved: editor.service.edit("dateY",Math.round(value)) }
        CheckBox { text: qsTr("Show date"); checked: editor.service.design.showDate; onToggled: editor.service.edit("showDate",checked) }
        CheckBox { text: qsTr("Live reload imported file"); checked: editor.service.liveReload; onToggled: editor.service.liveReload = checked }
        Caption { text: editor.service.state; Layout.fillWidth: true }
        Flow {
            Layout.fillWidth: true; Layout.preferredHeight: childrenRect.height; spacing: 10
            AppButton { text: qsTr("Save design"); onClicked: editor.service.save() }
            AppButton { text: qsTr("Import…"); onClicked: importFile.open() }
            AppButton { text: qsTr("Export…"); onClicked: exportFile.open() }
            AppButton { text: qsTr("Install on watch"); primary: true; available: editor.service.supported; onClicked: editor.service.install() }
        }
    }
}
