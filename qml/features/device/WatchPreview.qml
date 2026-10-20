import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Flint.Desktop

Rectangle {
    property bool blockPageGestures: true
    id: preview
    required property var session
    property var screenService: typeof watchScreen === "undefined" ? null : watchScreen
    activeFocusOnTab: true
    TapHandler { onTapped: preview.forceActiveFocus() }
    Keys.onPressed: event => {
        event.accepted = false;
        if (event.modifiers & (Qt.AltModifier | Qt.ControlModifier | Qt.MetaModifier)) return;
        if (!preview.session.connected || preview.session.transport !== "USB") return;
        const commands = {16777234:"previous",16777235:"previous",16777236:"next",16777237:"next",16777220:"enter",16777221:"enter",16777216:"back",83:"settings",78:"next",80:"previous",43:"plus",45:"minus"};
        const command=commands[event.key];
        if (command) { if (preview.screenService && preview.screenService.supported) preview.screenService.sendKey(command); else if (!preview.session.busy) preview.session.request("KEY",command); event.accepted=true; }
    }
    property bool frameVisible: true
    property bool compact: false
    property int zoom: 0
    readonly property bool hasFrame: screenService !== null && screenService.available
    implicitWidth: 300
    implicitHeight: compact ? 300 : 420
    color: Theme.base
    radius: 12
    border.color: Theme.divider
    FileDialog {
        id: screenshotFile
        title: qsTr("Save and verify actual watch pixels")
        fileMode: FileDialog.SaveFile
        nameFilters: ["PNG (*.png)"]
        defaultSuffix: "png"
        onAccepted: if (preview.screenService) preview.screenService.capture(selectedFile)
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: preview.compact ? 12 : 16
        spacing: 10
        RowLayout {
            Layout.fillWidth: true
            AppIcon { name: "watch"; width: 18; height: 18; tint: Theme.accent }
            Label { text: qsTr("Watch screen"); font.pixelSize: 13; font.weight: Font.DemiBold; Layout.fillWidth: true }
            Rectangle { width: 6; height: 6; radius: 3; color: preview.hasFrame && !preview.screenService.stale ? "#50d9b0" : Theme.offlineDot }
        }
        Flickable {
            id: viewport
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 160
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            contentWidth: Math.max(width,screenImage.width)
            contentHeight: Math.max(height,screenImage.height)
            interactive: preview.zoom > 0
            Image {
                id: screenImage
                visible: preview.hasFrame && preview.frameVisible
                source: preview.hasFrame && preview.frameVisible ? preview.screenService.source : ""
                cache: false
                asynchronous: false
                smooth: false
                mipmap: false
                width: preview.hasFrame ? preview.screenService.width * (preview.zoom > 0 ? preview.zoom : Math.min(viewport.width / preview.screenService.width,viewport.height / preview.screenService.height)) : 0
                height: preview.hasFrame ? preview.screenService.height * width / preview.screenService.width : 0
                x: Math.max(0,(viewport.width - width) / 2)
                y: Math.max(0,(viewport.height - height) / 2)
            }
            Caption {
                visible: !preview.hasFrame
                anchors.centerIn: parent
                width: parent.width - 12
                horizontalAlignment: Text.AlignHCenter
                text: preview.screenService && preview.screenService.state ? preview.screenService.state : qsTr("No confirmed screen received. No simulated image is shown.")
            }
        }
        Caption { visible: !preview.compact && preview.activeFocus; text: qsTr("Keyboard: arrows, Enter, Esc · S settings · + / −"); Layout.fillWidth: true; font.pixelSize: 10 }
        Caption {
            Layout.fillWidth: true
            font.pixelSize: 10
            text: preview.hasFrame ? (preview.screenService.stale ? qsTr("Last frame · no updates") : qsTr("Confirmed frame %1 · %2 s ago").arg(preview.screenService.revision).arg(preview.screenService.ageSeconds)) : qsTr("Requires compatible firmware and an authenticated connection")
        }
        RowLayout {
            visible: !preview.compact
            Layout.fillWidth: true
            AppComboBox { Layout.fillWidth: true; model: [qsTr("Fit"),"100%","200%"]; currentIndex: preview.zoom; onActivated: preview.zoom = currentIndex }
            AppButton { iconName: "sync"; text: ""; Accessible.name: qsTr("Refresh screen"); available: preview.screenService && preview.screenService.supported; onClicked: preview.screenService.refresh() }
            AppButton { iconName: "camera"; text: ""; Accessible.name: qsTr("Capture and compare pixels"); available: preview.screenService && preview.screenService.supported && !preview.screenService.capturing && preview.session.transport !== "BLE"; ToolTip.visible: hovered; ToolTip.text: qsTr("Read actual pixels over USB and verify the preview"); onClicked: screenshotFile.open() }
        }
        Caption { visible: preview.screenService !== null && preview.screenService.capturing; text: preview.screenService ? preview.screenService.verification : ""; Layout.fillWidth: true }
        AppButton { visible: preview.screenService !== null && preview.screenService.capturing; text: qsTr("Cancel capture"); onClicked: preview.screenService.cancelCapture() }
    }
}
