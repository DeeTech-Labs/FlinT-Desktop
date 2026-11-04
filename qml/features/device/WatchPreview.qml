import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Flint.Flare

Rectangle {
    id: card
    required property var session
    property bool compact: false
    property bool frameVisible: true
    property int zoom: 0
    implicitHeight: compact ? 280 : 410
    implicitWidth: 300
    color: Theme.surface
    radius: 12
    FileDialog {
        id: captureDialog
        fileMode: FileDialog.SaveFile
        nameFilters: ["PNG (*.png)"]
        defaultSuffix: "png"
        onAccepted: preview.capture(selectedFile)
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10
        RowLayout {
            Layout.fillWidth: true
            AppIcon {
                name: "watch"
                width: 18
                height: 18
                tint: Theme.accent
            }
            Label {
                text: qsTr("Watch screen")
                Layout.fillWidth: true
                font.weight: Font.DemiBold
            }
            Rectangle {
                width: 6
                height: 6
                radius: 3
                color: card.session.connected ? Theme.accent : Theme.offlineDot
            }
        }
        Image {
            source: "image://ignite/" + preview.revision
            sourceSize.width: 280
            sourceSize.height: 240
            fillMode: Image.PreserveAspectFit
            cache: false
            smooth: false
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 160
            visible: card.frameVisible
        }
        Caption {
            text: preview.status || qsTr("Connect a watch or start the local simulator")
            Layout.fillWidth: true
        }
        RowLayout {
            visible: !card.compact
            AppButton {
                iconName: "sync"
                text: qsTr("Refresh")
                available: card.session.connected
                onClicked: preview.refresh()
            }
            AppButton {
                iconName: "camera"
                text: qsTr("Save preview")
                available: card.session.connected
                onClicked: captureDialog.open()
            }
        }
    }
}
