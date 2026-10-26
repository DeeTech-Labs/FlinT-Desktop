import QtQuick
import QtQuick.Controls.Basic
import Flint.Desktop

ComboBox {
    property bool blockPageGestures: true
    id: control
    implicitHeight: Theme.controlHeight
    leftPadding: 13
    rightPadding: 34
    font.pixelSize: 13
    background: Rectangle { Behavior on border.color { ColorAnimation { duration: Theme.animationDuration } } radius: 11; color: Theme.input; border.color: control.activeFocus ? Theme.accent : Theme.inputBorder }
    indicator: AppIcon { name: "down"; width: 14; height: 14; tint: Theme.muted; x: control.width - width - 12; y: (control.height - height) / 2 }
    contentItem: Text { text: control.displayText; font: control.font; color: control.enabled ? Theme.text : Theme.muted; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight; textFormat: Text.PlainText }
    delegate: ItemDelegate {
        required property int index
        width: control.width - 12
        height: 39
        highlighted: control.highlightedIndex === index
        contentItem: Text { text: control.textAt(index); textFormat: Text.PlainText; color: Theme.text; font.pixelSize: 13; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight }
        background: Rectangle { radius: 7; color: parent.highlighted ? Theme.navSelected : "transparent" }
    }
    popup: Popup {
        y: control.height + 5
        width: control.width
        padding: 6
        implicitHeight: Math.min(300,contentItem.implicitHeight + 12)
        background: Rectangle { radius: 11; color: Theme.surface; border.color: Theme.inputBorder }
        contentItem: ListView { clip: true; implicitHeight: contentHeight; model: control.popup.visible ? control.delegateModel : null; currentIndex: control.highlightedIndex; ScrollIndicator.vertical: ScrollIndicator {} }
    }
}
