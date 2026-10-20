import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

Rectangle {
    id: hero
    required property var session
    required property var presentation
    property bool frameVisible: true
    property bool narrow: width < 640
    signal syncRequested
    signal settingsRequested
    implicitHeight: Math.max(narrow ? 350 : 370, heroContent.implicitHeight + (narrow ? 44 : 60))
    radius: 18
    border.color: Theme.divider
    gradient: Gradient {
        orientation: Gradient.Horizontal
        GradientStop { position: 0; color: Theme.heroStart }
        GradientStop { position: 1; color: Theme.heroEnd }
    }
    ColumnLayout {
        id: heroContent
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: hero.narrow ? 22 : 30
        width: parent.width - (hero.narrow ? 44 : 320)
        spacing: 14
        RowLayout {
            spacing: 7
            Rectangle { width: 6; height: 6; radius: 3; color: hero.session.connected ? "#50d9b0" : Theme.offlineDot }
            Label { text: hero.session.transport === "SIMULATOR" && hero.session.connected ? qsTr("Simulator") : hero.session.connected ? qsTr("Connected via %1").arg(hero.session.transport) : hero.presentation.stateLabel; color: Theme.heroText; font.pixelSize: 12 }
        }
        Label {
            text: hero.session.info.model ? (String(hero.session.info.model).toLowerCase().includes("flint") ? hero.session.info.model : "FlinT " + hero.session.info.model) : qsTr("Your watch")
            font.pixelSize: hero.narrow ? 32 : 38
            font.weight: Font.Bold
            color: Theme.text
            textFormat: Text.PlainText
            Layout.fillWidth: true
            elide: Text.ElideRight
        }
        Caption { text: hero.session.connected ? qsTr("Everything you need, connected.") : qsTr("Connect your watch to get started."); color: Theme.heroText; font.pixelSize: 14; Layout.fillWidth: true }
        GridLayout {
            columns: 2
            columnSpacing: 18; rowSpacing: 8
            Layout.topMargin: 10
            Caption { text: qsTr("Firmware"); color: Theme.heroFooter }
            Label { text: hero.presentation.value(hero.session.info,"firmware"); font.pixelSize: 13; color: Theme.text }
            Caption { text: qsTr("Watch face"); color: Theme.heroFooter }
            Label { text: hero.presentation.value(hero.session.status,"active_face"); font.pixelSize: 13; color: Theme.text }
            Caption { text: qsTr("Last synchronization"); color: Theme.heroFooter }
            Label { text: hero.session.lastSync ? hero.session.lastSync.substring(11,16) : "—"; font.pixelSize: 13; color: Theme.text }
        }
        Item { Layout.fillHeight: true }
        Flow {
            Layout.fillWidth: true
            Layout.preferredHeight: childrenRect.height
            spacing: 10
            AppButton { iconName: "sync"; text: qsTr("Sync time"); primary: true; available: hero.session.connected && (hero.session.info.capabilities || []).indexOf("time_sync") >= 0; onClicked: hero.syncRequested() }
            AppButton { iconName: "settings"; text: qsTr("Open settings"); onClicked: hero.settingsRequested() }
        }
    }
    WatchPreview {
        frameVisible: hero.frameVisible
        visible: !hero.narrow
        session: hero.session
        compact: true
        width: 280
        height: parent.height - 40
        anchors.right: parent.right
        anchors.rightMargin: 20
        anchors.verticalCenter: parent.verticalCenter
    }
}
