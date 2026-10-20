import QtQuick
import QtQuick.Controls.Basic
import Flint.Desktop

Item {
    id: glyph
    property string name: "watch"
    property color tint: Theme.muted
    implicitWidth: 22
    implicitHeight: 22
    // Qt's native icon renderer loads the packaged SVG and applies its tint.
    ToolButton {
        anchors.fill: parent
        enabled: false
        padding: 0
        focusPolicy: Qt.NoFocus
        display: AbstractButton.IconOnly
        icon.source: glyph.name ? "qrc:/icons/" + glyph.name + ".svg" : ""
        icon.color: glyph.tint
        icon.width: glyph.width
        icon.height: glyph.height
        background: null
    }
}
