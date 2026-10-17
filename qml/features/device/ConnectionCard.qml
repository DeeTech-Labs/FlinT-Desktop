import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

AppCard {
    required property var session
    implicitHeight: content.implicitHeight + topPadding + bottomPadding
    Layout.fillWidth: true
    ColumnLayout {
        id: content
        anchors.fill: parent
        spacing: 14
        Label {
            text: qsTr("Connection")
            font.pixelSize: 18
            font.bold: true
        }
        GridLayout {
            columns: width < 580 ? 2 : 3
            Layout.fillWidth: true
            columnSpacing: 10
            rowSpacing: 10
            AppComboBox {
                id: ports
                Accessible.name: qsTr("USB port")
                Layout.columnSpan: parent.columns === 2 ? 2 : 1
                Layout.minimumWidth: 0
                Layout.fillWidth: true
                implicitHeight: 42
                model: session.ports
                textRole: "label"
                valueRole: "path"
                enabled: !session.connected && !session.busy
                displayText: count ? currentText : qsTr("No USB devices found")
            }
            AppButton {
                busy: session.busy
                text: qsTr("Refresh")
                available: !session.connected
                onClicked: session.refreshPorts()
            }
            AppButton {
                primary: true
                available: session.connected || session.busy || ports.count > 0
                text: session.connected || session.busy ? qsTr("Disconnect") : qsTr("Connect")
                enabled: session.connected || session.busy || ports.count > 0
                onClicked: {
                    if (session.connected || session.busy)
                        session.disconnectDevice();
                    else
                        session.connectPort(ports.currentValue);
                }
            }
        }
        CheckBox {
            text: qsTr("Connect automatically")
            checked: session.autoConnect
            onToggled: session.autoConnect = checked
        }
        Caption {
            text: qsTr("Spark connects automatically over USB. For other adapters, select a port manually. Close other serial monitors and sync assistants.")
            Layout.fillWidth: true
        }
    }
}
