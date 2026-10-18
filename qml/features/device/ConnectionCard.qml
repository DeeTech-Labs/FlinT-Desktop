import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

AppCard {
    id: card
    property bool wireless: false
    function updateTransport() { if (session && session.connected && session.transport !== "SIMULATOR") wireless = session.transport === "BLE" }
    onSessionChanged: updateTransport()
    Component.onCompleted: updateTransport()
    Connections { target: card.session; function onStateChanged() { card.updateTransport() } }
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
        AppComboBox {
            Layout.fillWidth: true
            currentIndex: card.wireless ? 1 : 0
            displayText: session.transport === "SIMULATOR" && session.connected ? qsTr("Simulator") : currentText
            model: [qsTr("USB connection"), qsTr("Bluetooth LE")]
            enabled: !session.connected && !session.busy && !session.connecting
            onActivated: card.wireless = currentIndex === 1
        }
        GridLayout {
            columns: width < 580 ? 2 : 3
            Layout.fillWidth: true
            columnSpacing: 10
            rowSpacing: 10
            AppComboBox {
                id: ports
                Accessible.name: card.wireless ? qsTr("Bluetooth device") : qsTr("USB port")
                Layout.columnSpan: parent.columns === 2 ? 2 : 1
                Layout.minimumWidth: 0
                Layout.fillWidth: true
                implicitHeight: 42
                visible: session.transport !== "SIMULATOR" || !session.connected
                model: card.wireless ? session.bleDevices : session.ports
                textRole: "label"
                valueRole: "path"
                enabled: !session.connected && !session.busy && !session.connecting
                displayText: count ? currentText : (card.wireless ? qsTr("Scan for nearby Spark devices") : qsTr("No USB devices found"))
            }
            AppButton {
                busy: session.busy
                visible: session.transport !== "SIMULATOR" || !session.connected
                text: session.bleScanning ? qsTr("Scanning…") : qsTr("Refresh")
                available: !session.connected
                onClicked: card.wireless ? session.scanBle() : session.refreshPorts()
            }
            AppButton {
                primary: true
                available: session.connected || session.busy || session.connecting || ports.count > 0
                text: session.connected || session.busy || session.connecting ? qsTr("Disconnect") : qsTr("Connect")
                enabled: session.connected || session.busy || session.connecting || ports.count > 0
                onClicked: {
                    if (session.connected || session.busy || session.connecting)
                        session.disconnectDevice();
                    else if (card.wireless)
                        session.connectBle(ports.currentValue);
                    else
                        session.connectPort(ports.currentValue);
                }
            }
        }
        CheckBox {
            visible: session.transport !== "SIMULATOR" || !session.connected
            text: qsTr("Connect automatically")
            checked: session.autoConnect
            onToggled: session.autoConnect = checked
        }
        Caption {
            text: card.wireless ? qsTr("Enable Bluetooth on your computer, scan and select Spark. Accept the system pairing request if prompted.") : qsTr("Spark connects automatically over USB. For other adapters, select a port manually. Close other serial monitors and sync assistants.")
            Layout.fillWidth: true
        }
    }
}
