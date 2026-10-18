import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

AppPage {
    id: page
    required property var service
    required property var studioService
    AppCard {
        Layout.fillWidth: true
        implicitHeight: introduction.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: introduction; anchors.fill: parent; spacing: 10
            Label { text: qsTr("Watch faces"); font.pixelSize: 24; font.bold: true; Layout.fillWidth: true }
            Caption { text: qsTr("Faces reported by your firmware. Selecting a face requires device confirmation."); Layout.fillWidth: true }
            AppButton { text: qsTr("Refresh faces"); available: page.service.supported; onClicked: page.service.refresh() }
        }
    }
    Repeater {
        model: page.service.faces
        delegate: AppCard {
            required property var modelData
            Layout.fillWidth: true
            implicitHeight: faceContent.implicitHeight + topPadding + bottomPadding
            ColumnLayout {
                id: faceContent; anchors.fill: parent; spacing: 12
                Label { text: modelData.name; textFormat: Text.PlainText; font.pixelSize: 24; Layout.fillWidth: true }
                Caption { text: modelData.author + " · " + modelData.id; Layout.fillWidth: true }
                AppButton { text: page.service.values.active_face === modelData.id ? qsTr("Selected") : qsTr("Use on watch"); primary: true; available: page.service.supported && page.service.values.active_face !== modelData.id; onClicked: page.service.selectFace(modelData.id) }
            }
        }
    }
    FaceStudioEditor { service: page.studioService }
}
