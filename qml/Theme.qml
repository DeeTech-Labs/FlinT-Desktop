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
    property Settings preferences: Settings { category: "appearance"; property int mode: 0; property bool developerMode: false; property bool animations: true }
    readonly property bool light: mode === 1 || (mode === 0 && Qt.styleHints.colorScheme === Qt.Light)
    readonly property color background: light ? "#f3f4ef" : "#101112"
    readonly property color text: light ? "#182019" : "#f5f6f3"
    readonly property color muted: light ? "#58615b" : "#93979b"
    readonly property color accent: light ? "#486923" : "#d4fa70"
    readonly property color base: light ? "#ffffff" : "#202224"
    readonly property color button: light ? "#e4e8de" : "#292c2e"
    readonly property color accentText: "#141810"
    readonly property color primaryText: light ? "#ffffff" : "#17200b"
    readonly property color buttonText: light ? "#182019" : "#f1f3ee"
    readonly property color primaryPressed: "#b8dc59"
    readonly property color primaryHover: "#e1ff96"
    readonly property color buttonHover: light ? "#d7dfce" : "#373b3d"
    readonly property color surface: light ? "#ffffff" : "#1b1d1f"
    readonly property color metric: light ? "#f0f3eb" : "#232628"
    readonly property color sidebar: light ? "#e9ede3" : "#151718"
    readonly property color navText: light ? "#4d594d" : "#a6aaad"
    readonly property color navSelected: light ? "#d5e5be" : "#293021"
    readonly property color navHover: light ? "#dfe5d6" : "#222527"
    readonly property color divider: light ? "#d8dfd1" : "#2a2d2e"
    readonly property color offlineDot: "#777c80"
    readonly property color connectedSurface: light ? "#e0ecd2" : "#293323"
    readonly property color offlineSurface: light ? "#e8ece3" : "#242729"
    readonly property color heroStart: light ? "#d9e8c6" : "#303c2c"
    readonly property color heroEnd: light ? "#e5ede0" : "#1c2725"
    readonly property color heroDecoration: light ? "#c9dbb7" : "#354335"
    readonly property color watchBody: "#111715"
    readonly property color watchBorder: "#66785b"
    readonly property color watchTime: "#d4fa70"
    readonly property color watchScreen: "#202d23"
    readonly property color watchCaption: "#a4b695"
    readonly property color heroCaption: light ? "#476039" : "#c1d0b4"
    readonly property color heroText: light ? "#40513c" : "#c1ccbc"
    readonly property color heroFooter: light ? "#4b6040" : "#b1c1a6"
    readonly property color input: light ? "#f4f6ef" : "#25282a"
    readonly property color inputBorder: light ? "#c5cebd" : "#393d3f"
    readonly property color error: light ? "#a72c25" : "#ffaaa0"
    readonly property color logText: light ? "#2c402d" : "#c0c9bb"
    readonly property color logBackground: light ? "#eef2e8" : "#121516"
    readonly property color transparent: "transparent"
    readonly property string fontFamily: Qt.application.font.family
    readonly property string monoFamily: Qt.platform.os === "windows" ? "Consolas" : Qt.platform.os === "osx" ? "Menlo" : "monospace"
    readonly property int compactWidth: 900
    readonly property int pageMargin: 24
    readonly property int spacing: 16
    readonly property int cardRadius: 20
    readonly property int controlHeight: 42
    readonly property int animationDuration: animations ? 140 : 0
}
