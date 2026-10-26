import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Flint.Desktop

AppCard {
    property bool blockPageGestures: true
    id: editor
    required property var service
    property bool frameVisible: true
    property bool animationFrame: false
    Timer { interval: 500; repeat: true; running: editor.visible && editor.frameVisible && Theme.animations && editor.service.layers.animateIcon; onTriggered: editor.animationFrame = !editor.animationFrame }
    Layout.fillWidth: true
    implicitHeight: content.implicitHeight + topPadding + bottomPadding
    FileDialog { id: importFile; fileMode: FileDialog.OpenFile; nameFilters: ["FlinT face (*.flintface *.json)"]; onAccepted: editor.service.importDesign(selectedFile) }
    FileDialog { id: exportFile; fileMode: FileDialog.SaveFile; nameFilters: ["FlinT face (*.flintface)"]; defaultSuffix: "flintface"; onAccepted: editor.service.exportDesign(selectedFile) }
    ColorDialog { id: backgroundPicker; selectedColor: editor.service.design.background; onAccepted: editor.service.edit("background",selectedColor.toString()) }
    ColorDialog { id: foregroundPicker; selectedColor: editor.service.design.foreground; onAccepted: editor.service.edit("foreground",selectedColor.toString()) }
    ColumnLayout {
        id: content; anchors.fill: parent; spacing: 14
        Label { text: "Face Studio / Pixel Lab"; font.pixelSize: 24; font.weight: Font.DemiBold; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        Caption { text: qsTr("Create a clock layout with a 16 × 16 pixel icon. Left button draws, right button erases. Preview uses sample time."); Layout.fillWidth: true }
        AppComboBox { id: designs; model: editor.service.library.map(entry => (entry.favorite ? "★ " : "") + entry.design.name); Layout.fillWidth: true; onActivated: editor.service.load(currentIndex) }
        AppTextField { text: editor.service.design.name; maximumLength: 64; Layout.fillWidth: true; Accessible.name: qsTr("Design name"); onEditingFinished: editor.service.edit("name",text) }
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
                            color: (parseInt(editor.service.framePixels.charAt(Math.floor(index / 4)),16) & (8 >> (index % 4))) ? editor.service.design.foreground : editor.service.design.background
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
            ColumnLayout {
                Layout.preferredWidth: 280; Layout.alignment: Qt.AlignHCenter
                Image {
                    visible: editor.frameVisible
                    Layout.preferredWidth: 280; Layout.preferredHeight: 240
                    source: typeof designPreviewAvailable === "undefined" ? "" : "image://designpreview/" + encodeURIComponent(JSON.stringify(editor.service.design)) + "/" + (editor.animationFrame ? "1" : "0")
                    cache: false; smooth: false; asynchronous: false
                }
                Caption { text: qsTr("Design draft · sample values · watch renderer"); Layout.fillWidth: true }
            }
        }
        Flow {
            Layout.fillWidth: true; Layout.preferredHeight: childrenRect.height; spacing: 10
            AppButton { text: qsTr("Background"); onClicked: backgroundPicker.open() }
            AppButton { text: qsTr("Foreground"); onClicked: foregroundPicker.open() }
            AppButton { text: qsTr("Favorite"); onClicked: editor.service.favorite(designs.currentIndex) }
        }
        Label { text: qsTr("Layers"); font.pixelSize: 20; font.weight: Font.DemiBold; Layout.fillWidth: true }
        Caption { text: qsTr("Three layers: icon, time and date. Layer order is bottom to top. Animation switches between two icon frames every 500 ms."); Layout.fillWidth: true }
        AppComboBox {
            Layout.fillWidth: true
            model: [qsTr("Icon → Time → Date"), qsTr("Icon → Date → Time"), qsTr("Time → Icon → Date"), qsTr("Time → Date → Icon"), qsTr("Date → Icon → Time"), qsTr("Date → Time → Icon")]
            currentIndex: editor.service.layers.order
            onActivated: editor.service.editLayer("order",currentIndex)
        }
        AppSwitch { text: qsTr("Show icon"); checked: editor.service.layers.showIcon; onToggled: editor.service.editLayer("showIcon",checked) }
        Caption { text: qsTr("Icon horizontal position"); Layout.fillWidth: true }
        Slider { from: 0; to: 280 - 16 * editor.service.layers.iconScale; stepSize: 1; value: editor.service.layers.iconX; Layout.fillWidth: true; onMoved: editor.service.editLayer("iconX",Math.round(value)) }
        Caption { text: qsTr("Icon vertical position"); Layout.fillWidth: true }
        Slider { from: 0; to: 240 - 16 * editor.service.layers.iconScale; stepSize: 1; value: editor.service.layers.iconY; Layout.fillWidth: true; onMoved: editor.service.editLayer("iconY",Math.round(value)) }
        Caption { text: qsTr("Icon scale"); Layout.fillWidth: true }
        Slider { from: 1; to: 6; stepSize: 1; value: editor.service.layers.iconScale; Layout.fillWidth: true; onMoved: editor.service.editLayer("iconScale",Math.round(value)) }
        AppComboBox { Layout.fillWidth: true; model: [qsTr("Icon frame 1"),qsTr("Icon frame 2")]; currentIndex: editor.service.frame; onActivated: editor.service.setFrame(currentIndex) }
        AppSwitch { text: qsTr("Animate icon"); checked: editor.service.layers.animateIcon; onToggled: editor.service.editLayer("animateIcon",checked) }
        Caption { text: qsTr("Time position"); Layout.fillWidth: true }
        Slider { from: 80; to: 150; stepSize: 1; value: editor.service.design.timeY; Layout.fillWidth: true; onMoved: editor.service.edit("timeY",Math.round(value)) }
        Caption { text: qsTr("Date position"); Layout.fillWidth: true }
        Slider { from: 170; to: 220; stepSize: 1; value: editor.service.design.dateY; Layout.fillWidth: true; onMoved: editor.service.edit("dateY",Math.round(value)) }
        AppSwitch { Layout.fillWidth: true; Layout.minimumWidth: 0; Component.onCompleted: contentItem.wrapMode = Text.WordWrap; text: qsTr("Show date"); checked: editor.service.design.showDate; onToggled: editor.service.edit("showDate",checked) }
        AppSwitch { Layout.fillWidth: true; Layout.minimumWidth: 0; Component.onCompleted: contentItem.wrapMode = Text.WordWrap; text: qsTr("Live reload imported file"); checked: editor.service.liveReload; onToggled: editor.service.liveReload = checked }
        Caption { text: editor.service.state; Layout.fillWidth: true }
        Flow {
            Layout.fillWidth: true; Layout.preferredHeight: childrenRect.height; spacing: 10
            AppButton { iconName: "save"; text: qsTr("Save design"); onClicked: editor.service.save() }
            AppButton { iconName: "save"; text: qsTr("Import…"); onClicked: importFile.open() }
            AppButton { iconName: "save"; text: qsTr("Export…"); onClicked: exportFile.open() }
            AppButton { iconName: "watch"; text: qsTr("Install on watch"); primary: true; available: editor.service.supported; onClicked: editor.service.install() }
        }
    }
}
