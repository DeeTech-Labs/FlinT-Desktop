import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Flare

AppCard {
    id: card
    required property var session
    implicitHeight: content.implicitHeight + topPadding + bottomPadding
    ColumnLayout {
        id: content
        anchors.fill: parent
        spacing: 16
        Label {
            text: qsTr("Connect your watch")
            font.pixelSize: 22
            font.weight: Font.DemiBold
        }
        Caption {
            text: card.session.status
            Layout.fillWidth: true
        }
        Flow {
            Layout.fillWidth: true
            Layout.preferredHeight: childrenRect.height
            spacing: 10
            AppComboBox {
                id: ports
                width: 260
                model: card.session.ports
                textRole: "label"
                valueRole: "path"
            }
            AppButton {
                iconName: "usb"
                text: "USB"
                available: ports.count > 0
                onClicked: card.session.connectUsb(ports.currentValue)
            }
            AppButton {
                iconName: "bluetooth"
                text: qsTr("Scan Bluetooth")
                onClicked: card.session.scanBle()
            }
            AppComboBox {
                id: ble
                width: 260
                model: card.session.bleDevices
                textRole: "label"
                valueRole: "path"
            }
            AppButton {
                iconName: "bluetooth"
                text: "BLE"
                available: ble.count > 0
                onClicked: card.session.connectBle(ble.currentValue)
            }
        }
        RowLayout {
            AppButton {
                iconName: "play"
                text: qsTr("Start simulator")
                onClicked: card.session.startSimulator()
            }
            AppButton {
                iconName: "close"
                text: qsTr("Disconnect")
                available: card.session.connected
                onClicked: card.session.disconnectDevice()
            }
        }
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.divider
        }
        Label {
            text: qsTr("Trust a new Bluetooth device")
            font.pixelSize: 17
        }
        Caption {
            text: qsTr("Open pairing over USB, compare the code, and confirm on both sides.")
            Layout.fillWidth: true
        }
        Flow {
            Layout.fillWidth: true
            Layout.preferredHeight: childrenRect.height
            spacing: 10
            AppButton {
                text: qsTr("Open pairing for 60 seconds")
                available: card.session.localTrustChannel
                onClicked: card.session.pairing("open")
            }
            AppTextField {
                id: code
                width: 180
                text: card.session.watchPairingCode
                placeholderText: qsTr("Six-digit code")
                maximumLength: 6
                validator: RegularExpressionValidator {
                    regularExpression: /[0-9]{6}/
                }
            }
            AppButton {
                text: qsTr("Confirm on watch")
                available: card.session.localTrustChannel && code.acceptableInput
                onClicked: card.session.pairing("confirm=" + code.text)
            }
            AppButton {
                text: qsTr("Reject")
                available: card.session.localTrustChannel
                onClicked: card.session.pairing("reject")
            }
        }
    }
}
