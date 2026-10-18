import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

ScrollView {
    id: page
    Layout.minimumWidth: 0
    Layout.maximumWidth: parent ? parent.width : Infinity
    default property alias pageContent: body.data
    contentWidth: availableWidth
    clip: true
    contentData: ColumnLayout {
        id: body
        width: page.availableWidth
        spacing: Theme.spacing
    }
}
