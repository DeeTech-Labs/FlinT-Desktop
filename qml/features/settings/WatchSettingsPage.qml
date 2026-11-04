import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

AppPage {
    id: page
    required property var session
    AppCard {
        Layout.fillWidth: true
        implicitHeight: content.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: content
            anchors.fill: parent
            spacing: 14
            RowLayout {
                AppIcon {
                    name: "clock"
                    width: 22
                    height: 22
                    tint: Theme.accent
                }
                Label {
                    text: qsTr("Time and display")
                    font.pixelSize: 21
                    font.weight: Font.DemiBold
                }
            }
            Caption {
                text: qsTr("Set the UTC offset in minutes. Clock, theme and language are provided by Ignite packages.")
                Layout.fillWidth: true
            }
            Flow {
                Layout.fillWidth: true
                Layout.preferredHeight: childrenRect.height
                spacing: 10
                AppTextField {
                    id: offset
                    width: 200
                    text: "180"
                    placeholderText: qsTr("UTC offset, minutes")
                    validator: IntValidator {
                        bottom: -720
                        top: 840
                    }
                }
                AppButton {
                    iconName: "check"
                    text: qsTr("Apply to watch")
                    primary: true
                    available: page.session.connected && offset.acceptableInput
                    onClicked: page.session.setOffset(offset.text)
                }
                AppButton {
                    iconName: "sync"
                    text: qsTr("Sync time")
                    available: page.session.connected
                    onClicked: page.session.syncTime()
                }
            }
            AppButton {
                iconName: "home"
                text: qsTr("Restore factory components")
                available: page.session.connected
                onClicked: page.session.recover()
            }
        }
    }
    StatusCard {
        session: page.session
        Layout.fillWidth: true
    }
    WatchPreview {
        session: page.session
        Layout.fillWidth: true
        Layout.preferredHeight: 400
    }
}
