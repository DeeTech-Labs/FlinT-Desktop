import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

ComboBox {
    id: control
    implicitHeight: Theme.controlHeight
    leftPadding: 12
    rightPadding: 36
    background: Rectangle {
        radius: 10
        color: Theme.input
        border.color: control.activeFocus ? Theme.accent : Theme.inputBorder
    }
    indicator: Canvas {
        width: 16
        height: 16
        x: control.width - width - 12
        y: (control.height - height) / 2
        onPaint: {
            const context = getContext("2d");
            context.reset();
            context.strokeStyle = Theme.muted;
            context.lineWidth = 1.5;
            context.beginPath();
            context.moveTo(3, 5);
            context.lineTo(8, 10);
            context.lineTo(13, 5);
            context.stroke();
        }
    }
    contentItem: Text {
        text: control.displayText
        font: control.font
        color: Theme.text
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        textFormat: Text.PlainText
    }
}
