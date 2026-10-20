import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

AppPage {
    id: page
    required property var session
    required property var presentation
    signal connectRequested
    signal watchRequested
    signal facesRequested
    signal companionRequested(int tab)
    signal exportRequested
    DeviceHero {
        id: hero
        frameVisible: !page.inSwipeView || page.currentSwipePage
        Layout.fillWidth: true
        session: page.session
        presentation: page.presentation
        onSyncRequested: page.session.syncTime()
        onSettingsRequested: page.watchRequested()
    }
    WatchPreview { frameVisible: !page.inSwipeView || page.currentSwipePage; visible: hero.narrow; session: page.session; Layout.fillWidth: true; Layout.preferredHeight: 400 }
    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 10
        Label { text: qsTr("Quick actions"); font.pixelSize: 18; font.weight: Font.DemiBold; Layout.fillWidth: true }
        Caption { text: qsTr("Made for your everyday"); visible: page.width >= 600 }
    }
    GridLayout {
        Layout.fillWidth: true
        columns: width >= 650 ? 4 : 2
        columnSpacing: 12; rowSpacing: 12
        QuickAction { Layout.fillWidth: true; Layout.minimumWidth: 0; iconName: "grid"; text: qsTr("Watch faces"); detail: qsTr("Choose your style"); onClicked: page.facesRequested() }
        QuickAction { Layout.fillWidth: true; Layout.minimumWidth: 0; iconName: "note"; text: qsTr("Quick note"); detail: qsTr("Send a thought to your watch"); onClicked: page.companionRequested(0) }
        QuickAction { Layout.fillWidth: true; Layout.minimumWidth: 0; iconName: "focus"; text: qsTr("Focus"); detail: qsTr("Make time for what matters"); onClicked: page.companionRequested(1) }
        QuickAction { Layout.fillWidth: true; Layout.minimumWidth: 0; iconName: "connection"; text: qsTr("Connection"); detail: qsTr("Manage USB and Bluetooth"); onClicked: page.connectRequested() }
    }
    Rectangle {
        Layout.fillWidth: true
        implicitHeight: summary.implicitHeight + 32
        radius: 12; color: Theme.surface; border.color: Theme.divider
        RowLayout {
            id: summary
            anchors.fill: parent; anchors.margins: 16; spacing: 12
            AppIcon { name: "shield"; tint: "#57c9b0"; Layout.preferredWidth: 22; Layout.preferredHeight: 22 }
            Caption { Layout.fillWidth: true; text: page.session.connected && page.session.status.time_valid ? qsTr("Clock difference: %1 s · Last synchronization: %2").arg(page.session.clockDifference).arg(page.session.lastSync || qsTr("Not yet")) : qsTr("Your watch data stays on this computer.") }
            AppButton { iconName: "save"; text: qsTr("Save report…"); available: page.session.sampleCount > 0; onClicked: page.exportRequested() }
        }
    }
}
