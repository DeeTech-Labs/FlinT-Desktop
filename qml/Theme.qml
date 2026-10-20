pragma Singleton
import QtQuick
import QtCore

QtObject {
    property int mode: preferences.mode
    onModeChanged: preferences.mode = mode
    property bool developerMode: preferences.developerMode
    onDeveloperModeChanged: preferences.developerMode = developerMode
    property bool animations: preferences.animations
    onAnimationsChanged: preferences.animations = animations
    property int watchTab: navigation.watchTab
    onWatchTabChanged: navigation.watchTab = watchTab
    property int faceTab: navigation.faceTab
    onFaceTabChanged: navigation.faceTab = faceTab
    property int companionTab: navigation.companionTab
    onCompanionTabChanged: navigation.companionTab = companionTab
    property Settings navigation: Settings { category: "navigation"; property int watchTab: 0; property int faceTab: 0; property int companionTab: 0 }
    property Settings preferences: Settings { category: "appearance"; property int mode: 0; property bool developerMode: false; property bool animations: true }
    readonly property bool light: mode === 1 || (mode === 0 && Qt.styleHints.colorScheme === Qt.Light)
    readonly property color background: light ? "#f3f6fc" : "#0a1019"
    readonly property color text: light ? "#18243a" : "#edf3ff"
    readonly property color muted: light ? "#67768e" : "#879ab5"
    readonly property color accent: light ? "#2868ed" : "#3478ff"
    readonly property color base: light ? "#ffffff" : "#0c1421"
    readonly property color button: light ? "#e9eff9" : "#182638"
    readonly property color accentText: light ? "#ffffff" : "#ffffff"
    readonly property color primaryText: light ? "#ffffff" : "#ffffff"
    readonly property color buttonText: light ? "#263a58" : "#dbe7fb"
    readonly property color primaryPressed: light ? "#2054c5" : "#2054c5"
    readonly property color primaryHover: light ? "#4389ff" : "#4389ff"
    readonly property color buttonHover: light ? "#dce7fa" : "#22344d"
    readonly property color surface: light ? "#ffffff" : "#101b2a"
    readonly property color metric: light ? "#eef3fb" : "#142134"
    readonly property color sidebar: light ? "#eaf0fa" : "#0e1826"
    readonly property color navText: light ? "#526582" : "#9aacC6"
    readonly property color navSelected: light ? "#dce8ff" : "#1d3e77"
    readonly property color navHover: light ? "#e0e9f8" : "#17283e"
    readonly property color divider: light ? "#dde5f2" : "#203047"
    readonly property color offlineDot: light ? "#8090a7" : "#71829b"
    readonly property color connectedSurface: light ? "#e0eff8" : "#102e32"
    readonly property color offlineSurface: light ? "#eaf0f9" : "#162438"
    readonly property color heroStart: light ? "#f4f8ff" : "#101e31"
    readonly property color heroEnd: light ? "#e8f0ff" : "#0d1725"
    readonly property color heroDecoration: light ? "#dbe7ff" : "#173d76"
    readonly property color watchBody: light ? "#111b2c" : "#111b2c"
    readonly property color watchBorder: light ? "#65809f" : "#65809f"
    readonly property color watchTime: light ? "#ffffff" : "#ffffff"
    readonly property color watchScreen: light ? "#090f18" : "#090f18"
    readonly property color watchCaption: light ? "#99bae8" : "#99bae8"
    readonly property color heroCaption: light ? "#526e97" : "#91aed4"
    readonly property color heroText: light ? "#526582" : "#9bb0cd"
    readonly property color heroFooter: light ? "#647b9d" : "#7991b1"
    readonly property color input: light ? "#f3f6fc" : "#152337"
    readonly property color inputBorder: light ? "#d9e3f1" : "#253750"
    readonly property color error: light ? "#b83849" : "#ff8194"
    readonly property color logText: light ? "#385276" : "#b5c9e6"
    readonly property color logBackground: light ? "#f0f5fc" : "#09121e"
    readonly property color transparent: "transparent"
    readonly property string fontFamily: Qt.application.font.family
    readonly property string monoFamily: Qt.platform.os === "windows" ? "Consolas" : Qt.platform.os === "osx" ? "Menlo" : "monospace"
    readonly property int compactWidth: 900
    readonly property int pageMargin: 24
    readonly property int spacing: 16
    readonly property int cardRadius: 16
    readonly property int controlHeight: 44
    readonly property int animationDuration: animations ? 100 : 0
}
