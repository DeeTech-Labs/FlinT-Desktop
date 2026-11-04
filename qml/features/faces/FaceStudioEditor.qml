import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Flint.Flare

AppCard {
    implicitHeight: content.implicitHeight + topPadding + bottomPadding
    FileDialog {
        id: project
        title: qsTr("Select Ignite project")
        nameFilters: ["JSON (*.json)"]
    }
    FileDialog {
        id: output
        title: qsTr("Save Ignite")
        fileMode: FileDialog.SaveFile
        nameFilters: ["Ignite (*.ignit)"]
        defaultSuffix: "ignit"
        onAccepted: studio.build(project.selectedFile, selectedFile)
    }
    ColumnLayout {
        id: content
        anchors.fill: parent
        spacing: 14
        RowLayout {
            AppIcon {
                name: "edit"
                width: 22
                height: 22
                tint: Theme.accent
            }
            Label {
                text: "Ignite Studio"
                font.pixelSize: 21
                font.weight: Font.DemiBold
            }
        }
        Caption {
            text: qsTr("Compile a JSON project with its own screens, images, font subsets and data into a binary Ignite.")
            Layout.fillWidth: true
        }
        Caption {
            text: qsTr("Font and image compilation requires Python and Pillow.")
            Layout.fillWidth: true
        }
        Caption {
            text: project.selectedFile.toString()
            Layout.fillWidth: true
        }
        Flow {
            Layout.fillWidth: true
            Layout.preferredHeight: childrenRect.height
            spacing: 10
            AppButton {
                iconName: "search"
                text: qsTr("Choose project")
                onClicked: project.open()
            }
            AppButton {
                iconName: "check"
                text: qsTr("Build Ignite")
                primary: true
                busy: studio.busy
                available: project.selectedFile.toString().length > 0
                onClicked: output.open()
            }
            AppButton {
                iconName: "save"
                text: qsTr("Install build")
                available: client.connected && !studio.busy && studio.output.toString().length > 0
                onClicked: hub.install(studio.output)
            }
        }
        Caption {
            text: studio.message
            Layout.fillWidth: true
        }
    }
}
