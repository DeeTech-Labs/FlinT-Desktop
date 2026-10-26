import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

AppCard {
    id: card
    property bool wireless: false
    property int pairingRequest: 0
    property string pairingMessage: ""
    property var trustedPeers: []
    function managePairing(arguments) {
        pairingRequest = session.request("PAIR", arguments)
    }
    function updateTransport() { if (session && session.connected && session.transport !== "SIMULATOR") wireless = session.transport === "BLE" }
    onSessionChanged: updateTransport()
    Component.onCompleted: updateTransport()
    Connections {
        target: card.session
        function onStateChanged() { card.updateTransport(); if (!card.session.connected) { card.pairingRequest = 0; card.trustedPeers = []; card.pairingMessage = "" } }
        function onRequestFinished(id, okay, result, error) {
            if (id !== card.pairingRequest) return;
            card.pairingRequest = 0;
            if (!okay) { card.pairingMessage = error; return; }
            card.trustedPeers = result.peers || [];
            card.pairingMessage = result.open ? qsTr("Pairing window open for up to 60 seconds. Compare the codes in Desktop and the serial monitor.") : qsTr("Pairing window closed");
        }
    }
    property var screenService: typeof watchScreen === "undefined" ? null : watchScreen
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
            font.weight: Font.DemiBold
        }
        Rectangle {
            visible: !!card.session.error
            Layout.fillWidth: true
            implicitHeight: errorContent.implicitHeight + 24
            radius: 10; color: Theme.base; border.color: Theme.error
            ColumnLayout {
                id: errorContent; anchors.fill: parent; anchors.margins: 12; spacing: 8
                Label { text: qsTr("Connection needs attention"); font.weight: Font.DemiBold; color: Theme.error; Layout.fillWidth: true }
                Caption { text: card.session.error; Layout.fillWidth: true }
                Caption { text: card.wireless ? qsTr("Check Bluetooth, distance and pairing access. On this board, confirm pairing through the serial monitor.") : qsTr("Check the USB cable and close other serial monitors. Refresh the port list and reconnect."); Layout.fillWidth: true }
            }
        }
        AppComboBox {
            Layout.fillWidth: true
            currentIndex: card.wireless ? 1 : 0
            displayText: session.transport === "SIMULATOR" && session.connected ? qsTr("Simulator") : currentText
            model: [qsTr("USB connection"), qsTr("Bluetooth LE")]
            enabled: !session.connected && !session.interactionBusy && !session.connecting
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
                implicitHeight: Theme.controlHeight
                visible: session.transport !== "SIMULATOR" || !session.connected
                model: card.wireless ? session.bleDevices : session.ports
                textRole: "label"
                valueRole: "path"
                enabled: !session.connected && !session.interactionBusy && !session.connecting
                displayText: count ? currentText : (card.wireless ? qsTr("Scan for nearby Spark devices") : qsTr("No USB devices found"))
            }
            AppButton {
                busy: session.interactionBusy
                visible: session.transport !== "SIMULATOR" || !session.connected
                text: session.bleScanning ? qsTr("Scanning…") : qsTr("Refresh")
                available: !session.connected
                onClicked: card.wireless ? session.scanBle() : session.refreshPorts()
            }
            AppButton {
                primary: true
                available: session.connected || session.interactionBusy || session.connecting || ports.count > 0
                text: session.connected || session.interactionBusy || session.connecting ? qsTr("Disconnect") : qsTr("Connect")
                enabled: session.connected || session.interactionBusy || session.connecting || ports.count > 0
                onClicked: {
                    if (session.connected || session.interactionBusy || session.connecting)
                        session.disconnectDevice();
                    else if (card.wireless)
                        session.connectBle(ports.currentValue);
                    else
                        session.connectPort(ports.currentValue);
                }
            }
        }
        AppSwitch {
            visible: session.transport !== "SIMULATOR" || !session.connected
            text: qsTr("Connect automatically")
            checked: session.autoConnect
            onToggled: session.autoConnect = checked
        }
        Caption {
            text: card.wireless ? qsTr("Enable Bluetooth on your computer, scan and select Spark. Accept the system pairing request if prompted.") : qsTr("Spark connects automatically over USB. For other adapters, select a port manually. Close other serial monitors and sync assistants.")
            Layout.fillWidth: true
        }
        ColumnLayout {
            visible: card.session.connected && card.session.transport === "USB"
            Layout.fillWidth: true
            Label { text: qsTr("Trusted Bluetooth devices"); font.pixelSize: 18; font.weight: Font.DemiBold; Layout.fillWidth: true }
            Caption { text: qsTr("This board uses serial confirmation instead of a physical button. Disconnect USB in Desktop before connecting through BLE, then use the serial monitor to confirm the matching code."); Layout.fillWidth: true }
            Flow {
                Layout.fillWidth: true; Layout.preferredHeight: childrenRect.height; spacing: 10
                AppButton { iconName: "bluetooth"; text: qsTr("Open pairing window"); available: !card.session.interactionBusy && !card.pairingRequest; onClicked: card.managePairing("open") }
                AppButton { iconName: "close"; text: qsTr("Close pairing window"); available: !card.session.interactionBusy && !card.pairingRequest; onClicked: card.managePairing("reject") }
                AppButton { iconName: "sync"; text: qsTr("Refresh trusted devices"); available: !card.session.interactionBusy && !card.pairingRequest; onClicked: card.managePairing("status") }
            }
            Caption { text: card.pairingMessage; Layout.fillWidth: true }
            Repeater {
                model: card.trustedPeers
                delegate: RowLayout {
                    required property string modelData
                    Layout.fillWidth: true
                    Caption { text: modelData; Layout.fillWidth: true }
                    AppButton { iconName: "shield"; text: qsTr("Revoke access"); available: !card.session.interactionBusy && !card.pairingRequest; onClicked: card.managePairing("revoke=" + modelData) }
                }
            }
        }
    }
}
