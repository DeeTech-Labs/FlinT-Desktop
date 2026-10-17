import QtQuick
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
    ConnectionCard {
        Layout.fillWidth: true
        session: page.session
    }
    StatusCard {
        Layout.fillWidth: true
        session: page.session
        presentation: page.presentation
        onExportRequested: page.exportRequested()
    }
}
