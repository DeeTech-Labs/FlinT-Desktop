import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

AppPage {
    id: page
    required property var session
    required property var presentation
    DeviceInfoCard {
        session: page.session
        presentation: page.presentation
        Layout.fillWidth: true
    }
    WatchPreview {
        session: page.session
        Layout.fillWidth: true
        Layout.preferredHeight: 400
    }
    UsbLogCard {
        session: page.session
        Layout.fillWidth: true
    }
}
