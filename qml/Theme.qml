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
    readonly property color background: light ? "#e8edf5" : "#060a11"
    readonly property color text: light ? "#14243b" : "#f1f6ff"
    readonly property color muted: light ? "#62738c" : "#8799b4"
    readonly property color accent: light ? "#067c9e" : "#56def1"
    readonly property color base: light ? "#f8faff" : "#0b111c"
    readonly property color button: light ? "#e9eff6" : "#1a2639"
    readonly property color accentText: light ? "#ffffff" : "#082a3c"
    readonly property color primaryText: light ? "#ffffff" : "#082a3c"
    readonly property color buttonText: light ? "#334b68" : "#d7e5f7"
    readonly property color primaryPressed: light ? "#066c9f" : "#269fc9"
    readonly property color primaryHover: light ? "#1197d2" : "#70dbf5"
    readonly property color buttonHover: light ? "#dde8f3" : "#26374d"
    readonly property color surface: light ? "#ffffff" : "#111a29"
    readonly property color metric: light ? "#f2f6fb" : "#141f30"
    readonly property color sidebar: light ? "#e8edf5" : "#060a11"
    readonly property color navText: light ? "#61728c" : "#7990af"
    readonly property color navSelected: light ? "#cceef5" : "#d6f9ff"
    readonly property color navHover: light ? "#dae4f0" : "#16243a"
    readonly property color divider: light ? "#e5ebf4" : "#1c2a3d"
    readonly property color offlineDot: light ? "#8192aa" : "#7087a3"
    readonly property color connectedSurface: light ? "#deeffa" : "#143a4b"
    readonly property color offlineSurface: light ? "#eaf0f9" : "#182b41"
    readonly property color heroStart: light ? "#081a31" : "#081a31"
    readonly property color heroEnd: light ? "#050d1b" : "#050d1b"
    readonly property color heroDecoration: light ? "#b8e1fa" : "#235b83"
    readonly property color watchBody: light ? "#111b2c" : "#111b2c"
    readonly property color watchBorder: light ? "#65809f" : "#65809f"
    readonly property color watchTime: light ? "#ffffff" : "#ffffff"
    readonly property color watchScreen: light ? "#090f18" : "#090f18"
    readonly property color watchCaption: light ? "#99bae8" : "#99bae8"
    readonly property color heroCaption: light ? "#3f6a8b" : "#9ed6ee"
    readonly property color heroText: light ? "#a9bfd9" : "#a9bfd9"
    readonly property color heroFooter: light ? "#7896b8" : "#7896b8"
    readonly property color input: light ? "#f3f6fb" : "#0b1321"
    readonly property color inputBorder: light ? "#dce5f1" : "#263750"
    readonly property color error: light ? "#b83849" : "#ff8194"
    readonly property color logText: light ? "#385476" : "#b5cbe8"
    readonly property color logBackground: light ? "#f0f5fc" : "#091523"
    readonly property color transparent: "transparent"
    property FontLoader bodyFont: FontLoader { source: "qrc:/fonts/Lato-Regular.ttf" }
    property FontLoader semiboldFont: FontLoader { source: "qrc:/fonts/Lato-Semibold.ttf" }
    property FontLoader blackFont: FontLoader { source: "qrc:/fonts/Lato-Black.ttf" }
    readonly property string fontFamily: bodyFont.name || Qt.application.font.family
    readonly property string monoFamily: Qt.platform.os === "windows" ? "Consolas" : Qt.platform.os === "osx" ? "Menlo" : "monospace"
    readonly property int compactWidth: 900
    readonly property int pageMargin: 20
    readonly property int spacing: 14
    readonly property int cardRadius: 12
    readonly property int controlHeight: 36
    readonly property int animationDuration: animations ? 180 : 0
}
