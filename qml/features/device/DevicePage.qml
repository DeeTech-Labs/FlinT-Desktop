import QtQuick
import QtQuick.Layouts
import Flint.Desktop

AppPage {
    id: page
    required property var session
    required property var presentation
    signal exportRequested
    StatusCard {
        Layout.fillWidth: true
        session: page.session
        presentation: page.presentation
        onExportRequested: page.exportRequested()
    }
    DeviceInfoCard {
        Layout.fillWidth: true
        session: page.session
        presentation: page.presentation
    }
}
