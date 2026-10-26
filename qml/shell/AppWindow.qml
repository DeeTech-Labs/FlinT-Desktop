import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Flint.Desktop

ApplicationWindow {
    id: app
    width: 1320
    height: 880
    minimumWidth: 540
    minimumHeight: 480
    visible: true
    title: Qt.application.name
    color: Theme.background
    font.family: Theme.fontFamily
    font.pixelSize: 14
    property var screenService: typeof watchScreen === "undefined" ? null : watchScreen
    readonly property bool wantsScreen: visible && visibility !== Window.Minimized && (section === 0 || (section === 4 && diagnosticsPage.tab === 1))
    onWantsScreenChanged: if (screenService) screenService.active = wantsScreen
    onScreenServiceChanged: if (screenService) screenService.active = wantsScreen
    Component.onCompleted: if (screenService) screenService.active = wantsScreen
    property var session: device
    property var settingsService: watchSettings
    property var studioService: faceStudio
    property var toolsService: companionTools
    property var backgroundService: typeof backgroundAgent === "undefined" ? null : backgroundAgent
    onClosing: close => { if (backgroundService && backgroundService.enabled) { close.accepted = false; hide() } }
    property int section: 0
    property var navigationHistory: [0]
    property int historyPosition: 0
    property bool restoringHistory: false
    onSectionChanged: {
        pageReveal.restart();
        if (!restoringHistory && navigationHistory[historyPosition] !== section) {
            const next = navigationHistory.slice(0, historyPosition + 1);
            next.push(section); navigationHistory = next; historyPosition = next.length - 1;
        }
    }
    function pendingChanges() {
        const names = {hour_format:qsTr("Time format"),show_seconds:qsTr("Seconds"),timezone:qsTr("Time zone"),date_format:qsTr("Date format"),date_separator:qsTr("Date separator"),language:qsTr("Watch language"),active_face:qsTr("Watch face")};
        const labels = {hour_format:[qsTr("24 hours"),qsTr("12 hours")],show_seconds:{0:qsTr("Hide"),1:qsTr("Show"),255:qsTr("Automatic")},language:["Русский","English"],date_separator:[".","/","-"],date_format:["31.12.26","31.12.2026","12.31.26","12.31.2026","2026.12.31",qsTr("Month name")]};
        const changes=[];
        const draft=app.settingsService.draft, values=app.settingsService.values;
        for (const key of Object.keys(draft)) if (draft[key] !== values[key]) {
            const text = value => labels[key] && labels[key][value] !== undefined ? labels[key][value] : value;
            changes.push((names[key] || key) + ": " + text(values[key]) + " → " + text(draft[key]));
        }
        return changes;
    }
    function goBack() {
        if (historyPosition <= 0) return;
        restoringHistory = true; historyPosition--; section = navigationHistory[historyPosition]; restoringHistory = false;
    }
    function goForward() {
        if (historyPosition + 1 >= navigationHistory.length) return;
        restoringHistory = true; historyPosition++; section = navigationHistory[historyPosition]; restoringHistory = false;
    }
    Shortcut { context: Qt.ApplicationShortcut; sequences: [StandardKey.Back]; onActivated: app.goBack() }
    Shortcut { context: Qt.ApplicationShortcut; sequences: [StandardKey.Forward]; onActivated: app.goForward() }
    Shortcut { context: Qt.ApplicationShortcut; sequence: "Alt+1"; onActivated: app.section = 0 }
    Shortcut { context: Qt.ApplicationShortcut; sequence: "Alt+2"; onActivated: app.section = 1 }
    Shortcut { context: Qt.ApplicationShortcut; sequence: "Alt+3"; onActivated: app.section = 2 }
    Shortcut { context: Qt.ApplicationShortcut; sequence: "Alt+4"; onActivated: app.section = 3 }
    TapHandler { acceptedButtons: Qt.BackButton; onTapped: app.goBack() }
    TapHandler { acceptedButtons: Qt.ForwardButton; onTapped: app.goForward() }
    property real horizontalGesture: 0
    property double lastHorizontalNavigation: 0
    Timer { id: gestureReset; interval: 200; onTriggered: app.horizontalGesture = 0 }
    function navigationGestureAllowed(x,y) {
        function blocks(item,depth) {
            if (!item || depth>30 || !item.visible || item.opacity===0) return false;
            if (item.inSwipeView && !item.currentSwipePage) return false;
            const p=item.mapFromItem(app.contentItem,x,y);
            const inside=p.x>=0 && p.y>=0 && p.x<item.width && p.y<item.height;
            if (item.clip && !inside) return false;
            if (inside && item.blockPageGestures===true) return true;
            for (const child of item.children || []) if (blocks(child,depth+1)) return true;
            return false;
        }
        return !blocks(app.contentItem,0);
    }
    WheelHandler {
        target: null
        orientation: Qt.Horizontal
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        onWheel: event => {
            if (!app.navigationGestureAllowed(event.x,event.y)) { event.accepted = false; return; }
            const horizontal = event.pixelDelta.x || event.angleDelta.x;
            const vertical = event.pixelDelta.y || event.angleDelta.y;
            if (Math.abs(horizontal) <= Math.abs(vertical)) { event.accepted = false; return; }
            app.horizontalGesture += horizontal;
            gestureReset.restart();
            if (Math.abs(app.horizontalGesture) >= 70 && Date.now() - app.lastHorizontalNavigation > 400) {
                app.section = Math.max(0,Math.min(6,app.section + (app.horizontalGesture < 0 ? 1 : -1)));
                app.lastHorizontalNavigation = Date.now(); app.horizontalGesture = 0;
            }
            event.accepted = true;
        }
    }
    readonly property bool compact: width < Theme.compactWidth
    palette.window: Theme.surface
    palette.mid: Theme.inputBorder
    palette.dark: Theme.muted
    palette.placeholderText: Theme.muted
    palette.windowText: Theme.text
    palette.text: Theme.text
    palette.base: Theme.base
    palette.button: Theme.button
    palette.buttonText: Theme.text
    palette.highlight: Theme.accent
    palette.highlightedText: Theme.accentText

    DevicePresentation {
        id: devicePresentation
        session: app.session
    }
    FileDialog {
        id: exportDialog
        title: qsTr("Save captured data")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("JSON data (*.json)")]
        defaultSuffix: "json"
        onAccepted: app.session.saveData(selectedFile)
    }
    GridLayout {
        anchors.fill: parent
        anchors.margins: 8
        columns: app.compact ? 1 : 2
        columnSpacing: 0
        rowSpacing: 0
        AppNavigation {
            session: app.session
            compact: app.compact
            currentIndex: app.section
            onSelected: index => app.section = index
            Layout.fillWidth: app.compact
            Layout.fillHeight: !app.compact
            Layout.preferredWidth: app.compact ? -1 : 60
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.base
            radius: 18
            ColumnLayout {
            anchors.fill: parent
            anchors.margins: app.compact ? 14 : Theme.pageMargin
            spacing: Theme.spacing
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                AppButton { iconName: "left"; text: ""; available: app.historyPosition > 0; implicitWidth: 36; ToolTip.visible: hovered; ToolTip.text: qsTr("Back"); onClicked: app.goBack() }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Label {
                        text: [qsTr("Overview"),qsTr("My watch"),qsTr("Watch faces"),"Companion",qsTr("Diagnostics"),qsTr("Desktop settings"),qsTr("Connect your watch")][app.section]
                        font.pixelSize: app.compact ? 24 : 28
                        font.weight: Font.DemiBold
                        font.letterSpacing: -0.6
                        color: Theme.text
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }
                    Caption {
                        visible: app.section !== 0
                        text: [qsTr("Your watch, at a glance"),qsTr("Time, display and saved profiles"),qsTr("Choose, create and install watch faces"),qsTr("Notes, focus and computer metrics"),qsTr("Telemetry, screen inspection and reports"),qsTr("Personalize your desktop experience"),qsTr("One connection. Everything in sync.")][app.section]
                        Layout.fillWidth: true
                    }
                }
                AppButton {
                    iconName: app.session.connected && app.session.transport === "BLE" ? "bluetooth" : "connection"
                    text: app.compact ? "" : app.session.connected ? app.session.transport : qsTr("Connect")
                    implicitWidth: app.compact ? 44 : contentItem.implicitWidth + leftPadding + rightPadding
                    onClicked: app.section = 6
                }
            }
            SwipeView {
                id: pages
                objectName: "pageStack"
                Component.onCompleted: contentItem.highlightMoveDuration = 0
                clip: true
                onCurrentIndexChanged: if (app.section !== currentIndex) app.section = currentIndex
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: app.section
                OverviewPage {
                    session: app.session
                    presentation: devicePresentation
                    onConnectRequested: app.section = 6
                    onWatchRequested: app.section = 1
                    onFacesRequested: app.section = 2
                    onCompanionRequested: tab => { companionPage.tab = tab; app.section = 3 }
                }
                WatchSettingsPage { objectName: "watchPage"; service: app.settingsService; session: app.session; presentation: devicePresentation }
                FaceGalleryPage { objectName: "facesPage"; service: app.settingsService; studioService: app.studioService }
                CompanionPage { id: companionPage; objectName: "companionPage"; service: app.toolsService }
                DiagnosticsPage {
                    id: diagnosticsPage
                    objectName: "diagnosticsPage"
                    session: app.session
                    presentation: devicePresentation
                    onExportRequested: exportDialog.open()
                }
                DesktopSettingsPage { session: app.session; backgroundService: app.backgroundService }
                AppPage { ConnectionCard { session: app.session; Layout.fillWidth: true } }
            }
            Rectangle {
                objectName: "pendingSettingsBar"
                visible: app.settingsService.dirty
                Layout.fillWidth: true
                implicitHeight: pendingLayout.implicitHeight + 24
                radius: 12; color: Theme.navSelected; border.color: Theme.accent
                ColumnLayout {
                    id: pendingLayout
                    anchors.fill: parent; anchors.margins: 12; spacing: 8
                    Label { text: qsTr("Unapplied changes · %1").arg(app.pendingChanges().length); font.bold: true; Layout.fillWidth: true }
                    Caption { text: app.pendingChanges().join(" · "); Layout.fillWidth: true }
                    Flow {
                        Layout.fillWidth: true; Layout.preferredHeight: childrenRect.height; spacing: 8
                        AppButton { text: qsTr("Apply to watch"); iconName: "check"; primary: true; available: app.settingsService.supported && !app.session.interactionBusy; onClicked: app.settingsService.apply() }
                        AppButton { text: qsTr("Discard"); iconName: "close"; onClicked: app.settingsService.discard() }
                        Caption { text: app.settingsService.supported ? qsTr("Draft kept when switching pages") : qsTr("Reconnect this watch to apply"); width: 220 }
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                AppIcon { name: "shield"; width: 12; height: 12; tint: Theme.muted }
                Caption { text: qsTr("Data stays on this computer and your watch."); font.pixelSize: 10; Layout.fillWidth: true }
                Caption { text: "FlinT / Desktop"; font.pixelSize: 10; font.letterSpacing: 1 }
            }
        }
    }
    }

    NumberAnimation {
        id: pageReveal
        target: pages
        property: "opacity"
        from: 0.72; to: 1
        duration: Theme.animationDuration
        easing.type: Easing.OutCubic
    }

    Popup {
        id: pairingPrompt
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(app.width - 32, 480)
        modal: true
        closePolicy: Popup.NoAutoClose
        visible: !!app.session.pairingCode
        background: Rectangle { color: Theme.surface; radius: 16; border.color: Theme.inputBorder }
        contentItem: ColumnLayout {
            Label { text: qsTr("Confirm pairing"); font.pixelSize: 22; font.bold: true }
            Label { text: app.session.pairingCode || ""; font.pixelSize: 36; font.family: Theme.monoFamily }
            Caption { text: qsTr("Compare this code with the watch serial monitor. Confirm on both devices only when the codes match."); Layout.fillWidth: true }
            AppButton { text: qsTr("Codes match"); primary: true; onClicked: app.session.confirmPairing(true) }
            AppButton { text: qsTr("Reject"); onClicked: app.session.confirmPairing(false) }
        }
    }
    Popup {
        id: toast
        parent: Overlay.overlay
        x: Math.max(12, (app.width - width) / 2)
        y: app.height - height - 24
        width: Math.min(app.width - 32, 620)
        padding: 18
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle { color: Theme.surface; radius: 14; border.color: Theme.inputBorder }
        contentItem: Label {
            text: app.session.error.length ? app.session.error : app.session.notification
            color: app.session.error.length ? Theme.error : Theme.text
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
        }
        enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.animationDuration } }
        exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.animationDuration } }
        Timer { id: toastTimer; interval: 6500; onTriggered: toast.close() }
    }
    Connections {
        target: app.session
        property string previousMessage: ""
        Component.onCompleted: previousMessage = app.session.notification
        function onStateChanged() {
            const message = app.session.error || app.session.notification
            if (message && message !== previousMessage) { toast.open(); toastTimer.restart() }
            previousMessage = message
        }
    }
}
