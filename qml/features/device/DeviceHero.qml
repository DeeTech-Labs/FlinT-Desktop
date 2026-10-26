import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

Rectangle {
    id: hero
    required property var session
    required property var presentation
    property bool frameVisible: true
    readonly property bool narrow: width < 740
    signal syncRequested
    signal settingsRequested
    signal connectRequested
    implicitHeight: Math.max(260, words.implicitHeight + 40)
    radius: 18
    clip: true
    gradient: Gradient {
        orientation: Gradient.Horizontal
        GradientStop { position: 0; color: Theme.heroStart }
        GradientStop { position: 1; color: Theme.heroEnd }
    }
    HeroAtmosphere { anchors.fill: parent; active: hero.frameVisible }
    ColumnLayout {
        id: words
        anchors.left: parent.left; anchors.top: parent.top
        anchors.margins: 22
        width: hero.narrow ? parent.width - 44 : parent.width * 0.53
        spacing: 12
        RowLayout {
            spacing: 8
            Rectangle { width: 6; height: 6; radius: 3; color: hero.session.connected ? "#58e2c6" : "#6987a6" }
            Label {
                text: hero.session.transport === "SIMULATOR" && hero.session.connected ? qsTr("Simulator") : hero.session.connected ? qsTr("Connected via %1").arg(hero.session.transport) : hero.presentation.stateLabel
                font.pixelSize: 12; color: "#b2c8e1"
            }
        }
        Label {
            text: qsTr("Your time.\nYour own rhythm.")
            font.pixelSize: hero.narrow ? 34 : 42
            font.weight: Font.DemiBold; font.letterSpacing: -2
            lineHeight: 0.98
            color: "#f1f8ff"
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
        Label {
            text: hero.session.connected ? (hero.session.info.model || qsTr("Your watch")) : qsTr("A little device. A space of your own.")
            textFormat: Text.PlainText
            font.pixelSize: 13; color: "#a3bad6"
            Layout.fillWidth: true; wrapMode: Text.WordWrap
        }
        Flow {
            Layout.fillWidth: true; Layout.preferredHeight: childrenRect.height
            Layout.topMargin: 4
            spacing: 10
            AppButton { visible: hero.session.connected; iconName: "sync"; text: qsTr("Sync time"); primary: true; busy: hero.session.interactionBusy; available: (hero.session.info.capabilities || []).indexOf("time_sync") >= 0; onClicked: hero.syncRequested() }
            AppButton { visible: !hero.session.connected; iconName: "connection"; text: qsTr("Connect your watch"); primary: true; onClicked: hero.connectRequested() }
            AppButton { iconName: "settings"; text: qsTr("Make it yours"); onClicked: hero.settingsRequested() }
        }
    }
    Label {
        visible: !hero.narrow
        anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 20
        text: "F L I N T   /   " + (hero.session.status.active_face || "DESKTOP")
        font.pixelSize: 10; color: "#6888ad"; textFormat: Text.PlainText
    }
}
