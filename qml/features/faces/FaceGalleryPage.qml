import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Flint.Desktop

AppPage {
    id: page
    FileDialog {
        id: install
        title: qsTr("Install Ignite")
        nameFilters: ["Ignite (*.ignit)"]
        onAccepted: hub.install(selectedFile)
    }
    Dialog {
        id: consent
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(500, parent.width - 32)
        modal: true
        title: qsTr("Application permission")
        property string permissionName
        property string scopeName
        property bool enable
        standardButtons: Dialog.Ok | Dialog.Cancel
        contentItem: Label {
            text: hub.selected + "\n" + consent.permissionName + "\n" + (consent.scopeName || qsTr("Own data")) + "\n\n" + (consent.enable ? qsTr("Allow this access? You can revoke it here.") : qsTr("Revoke this access?"))
            wrapMode: Text.WordWrap
            textFormat: Text.PlainText
        }
        onAccepted: hub.grant(permissionName, scopeName, enable)
    }
    Dialog {
        id: removal
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(420, parent.width - 32)
        modal: true
        title: qsTr("Remove package?")
        property string packageId
        standardButtons: Dialog.Ok | Dialog.Cancel
        contentItem: Label {
            text: removal.packageId
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
        }
        onAccepted: hub.remove(packageId)
    }
    RowLayout {
        Layout.fillWidth: true
        Caption {
            text: hub.message
            Layout.fillWidth: true
        }
        ProgressBar {
            visible: hub.busy
            value: hub.progress
            Layout.preferredWidth: 120
        }
        AppButton {
            iconName: "sync"
            text: qsTr("Refresh")
            available: client.connected && !hub.busy
            onClicked: hub.refresh()
        }
        AppButton {
            iconName: "save"
            text: qsTr("Install Ignite")
            primary: true
            available: client.connected && !hub.busy
            onClicked: install.open()
        }
    }
    Repeater {
        model: hub.packages
        delegate: AppCard {
            id: packageCard
            required property var modelData
            Layout.fillWidth: true
            implicitHeight: content.implicitHeight + topPadding + bottomPadding
            ColumnLayout {
                id: content
                anchors.fill: parent
                spacing: 12
                RowLayout {
                    AppIcon {
                        name: "grid"
                        width: 22
                        height: 22
                        tint: Theme.accent
                    }
                    Label {
                        text: packageCard.modelData.name
                        textFormat: Text.PlainText
                        font.pixelSize: 20
                        font.weight: Font.DemiBold
                        Layout.fillWidth: true
                    }
                    Caption {
                        text: packageCard.modelData.factory ? qsTr("Factory") : qsTr("Installed")
                    }
                }
                Caption {
                    text: packageCard.modelData.id + " · " + packageCard.modelData.publisher + " · v" + packageCard.modelData.version
                    Layout.fillWidth: true
                }
                Flow {
                    Layout.fillWidth: true
                    Layout.preferredHeight: childrenRect.height
                    spacing: 8
                    Repeater {
                        model: packageCard.modelData.components
                        delegate: Row {
                            required property var modelData
                            spacing: 8
                            AppButton {
                                text: modelData.type === 3 ? qsTr("Apply theme") : modelData.type === 4 ? qsTr("Use language") : qsTr("Open %1").arg(modelData.id)
                                iconName: "play"
                                onClicked: {
                                    if (modelData.type === 3)
                                        hub.setRole(4, packageCard.modelData.id, modelData.id);
                                    else if (modelData.type === 4)
                                        hub.setRole(5, packageCard.modelData.id, modelData.id);
                                    else
                                        hub.launch(packageCard.modelData.id, modelData.id);
                                }
                            }
                            AppButton {
                                visible: modelData.type === 2
                                text: qsTr("Use as clock")
                                iconName: "watch"
                                onClicked: hub.setRole(1, packageCard.modelData.id, modelData.id)
                            }
                        }
                    }
                    AppButton {
                        iconName: "shield"
                        text: qsTr("Permissions")
                        onClicked: hub.select(packageCard.modelData.id)
                    }
                    AppButton {
                        visible: !packageCard.modelData.factory
                        iconName: "left"
                        text: qsTr("Roll back")
                        onClicked: hub.rollback(packageCard.modelData.id)
                    }
                    AppButton {
                        visible: !packageCard.modelData.factory
                        iconName: "close"
                        text: qsTr("Remove")
                        onClicked: {
                            removal.packageId = packageCard.modelData.id;
                            removal.open();
                        }
                    }
                }
            }
        }
    }
    AppCard {
        visible: hub.selected.length > 0
        Layout.fillWidth: true
        implicitHeight: permissions.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: permissions
            anchors.fill: parent
            spacing: 12
            Label {
                text: qsTr("Permissions") + " · " + hub.selected
                font.pixelSize: 20
                textFormat: Text.PlainText
                Layout.fillWidth: true
            }
            Repeater {
                model: hub.permissions
                delegate: RowLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    Caption {
                        text: modelData.name + (modelData.scope ? " · " + modelData.scope : "")
                        Layout.fillWidth: true
                    }
                    Caption {
                        text: [qsTr("Automatic"), qsTr("Persistent"), qsTr("Session"), qsTr("Selected resource"), qsTr("Explicit action"), qsTr("Restricted")][modelData.access] || qsTr("Unsupported")
                    }
                    AppButton {
                        text: modelData.enabled ? qsTr("Revoke") : qsTr("Allow")
                        available: modelData.access > 0 && modelData.access < 4
                        onClicked: {
                            consent.permissionName = modelData.name;
                            consent.scopeName = modelData.scope;
                            consent.enable = !modelData.enabled;
                            consent.open();
                        }
                    }
                }
            }
        }
    }
    FaceStudioEditor {
        Layout.fillWidth: true
    }
}
