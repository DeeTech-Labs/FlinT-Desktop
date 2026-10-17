import QtQuick
import QtQuick.Layouts
import Flint.Desktop

AppPage {
    id: page
    required property var session
    UsbLogCard {
        Layout.fillWidth: true
        session: page.session
    }
}
