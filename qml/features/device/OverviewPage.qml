import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

AppPage {
    id: page
    required property var session
    required property var presentation
    signal exportRequested
    DeviceHero {
        Layout.fillWidth: true
        session: page.session
        presentation: page.presentation
    }
    Caption {
        text: page.session.connected && page.session.status.time_valid ? qsTr("Clock difference: %1 s · Last synchronization: %2").arg(page.session.clockDifference).arg(page.session.lastSync || qsTr("Not yet")) : qsTr("Connect your watch to check time synchronization.")
        Layout.fillWidth: true
    }
    ConnectionCard {
        Layout.fillWidth: true
        session: page.session
    }
    AppCard {
        Layout.fillWidth: true
        implicitHeight: 82
        RowLayout {
            anchors.fill: parent
            AppButton { text: qsTr("Sync time"); primary: true; available: page.session.connected && !page.session.busy && (page.session.info.capabilities || []).indexOf("time_sync") >= 0; onClicked: page.session.syncTime() }
            AppButton { text: qsTr("Save report…"); available: page.session.sampleCount > 0; onClicked: page.exportRequested() }
        }
    }
}
