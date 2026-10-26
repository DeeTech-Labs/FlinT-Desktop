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
    DeviceHero {
        Layout.fillWidth: true
        frameVisible: !page.inSwipeView || page.currentSwipePage
        session: page.session; presentation: page.presentation
        onConnectRequested: page.connectRequested()
        onSyncRequested: page.session.syncTime()
        onSettingsRequested: page.watchRequested()
    }
    GridLayout {
        Layout.fillWidth: true
        columns: width >= 880 ? 2 : 1
        columnSpacing: 16; rowSpacing: 16
        ColumnLayout {
            Layout.fillWidth: true; Layout.minimumWidth: 0
            Layout.preferredWidth: 2
            spacing: 12
            RowLayout {
                Layout.fillWidth: true
                Label { text: qsTr("Make room for your day"); font.pixelSize: 18; font.weight: Font.Medium; font.letterSpacing: -0.5; Layout.fillWidth: true; wrapMode: Text.WordWrap }
            }
            GridLayout {
                Layout.fillWidth: true
                columns: width >= 510 ? 2 : 1
                columnSpacing: 12; rowSpacing: 12
                QuickAction { Layout.fillWidth: true; Layout.minimumWidth: 0; iconName: "grid"; text: qsTr("Watch faces"); detail: qsTr("Choose your style"); onClicked: page.facesRequested() }
                QuickAction { Layout.fillWidth: true; Layout.minimumWidth: 0; iconName: "note"; text: qsTr("Quick note"); detail: qsTr("Send a thought to your watch"); onClicked: page.companionRequested(0) }
                QuickAction { Layout.fillWidth: true; Layout.minimumWidth: 0; iconName: "focus"; text: qsTr("Focus"); detail: qsTr("Make time for what matters"); onClicked: page.companionRequested(1) }
                QuickAction { Layout.fillWidth: true; Layout.minimumWidth: 0; iconName: "connection"; text: qsTr("Connection"); detail: qsTr("Manage USB and Bluetooth"); onClicked: page.connectRequested() }
            }
        }
        WatchPreview {
            Layout.fillWidth: true; Layout.minimumWidth: 0
            Layout.preferredWidth: 1
            Layout.preferredHeight: 218
            session: page.session
            compact: true
            frameVisible: !page.inSwipeView || page.currentSwipePage
        }
    }
}
