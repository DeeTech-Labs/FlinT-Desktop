pragma Singleton
import QtQuick

QtObject {
    readonly property color background: "#101112"
    readonly property color text: "#f5f6f3"
    readonly property color muted: "#93979b"
    readonly property color accent: "#d4fa70"
    readonly property color base: "#202224"
    readonly property color button: "#292c2e"
    readonly property color accentText: "#141810"
    readonly property color primaryText: "#17200b"
    readonly property color buttonText: "#f1f3ee"
    readonly property color primaryPressed: "#b8dc59"
    readonly property color primaryHover: "#e1ff96"
    readonly property color buttonHover: "#373b3d"
    readonly property color surface: "#1b1d1f"
    readonly property color metric: "#232628"
    readonly property color sidebar: "#151718"
    readonly property color navText: "#a6aaad"
    readonly property color navSelected: "#293021"
    readonly property color navHover: "#222527"
    readonly property color divider: "#2a2d2e"
    readonly property color offlineDot: "#777c80"
    readonly property color connectedSurface: "#293323"
    readonly property color offlineSurface: "#242729"
    readonly property color heroStart: "#303c2c"
    readonly property color heroEnd: "#1c2725"
    readonly property color heroDecoration: "#354335"
    readonly property color watchBody: "#111715"
    readonly property color watchBorder: "#66785b"
    readonly property color watchScreen: "#202d23"
    readonly property color watchCaption: "#a4b695"
    readonly property color heroCaption: "#c1d0b4"
    readonly property color heroText: "#c1ccbc"
    readonly property color heroFooter: "#b1c1a6"
    readonly property color input: "#25282a"
    readonly property color inputBorder: "#393d3f"
    readonly property color error: "#ffaaa0"
    readonly property color logText: "#c0c9bb"
    readonly property color logBackground: "#121516"
    readonly property color transparent: "transparent"
    readonly property string fontFamily: Qt.application.font.family
    readonly property string monoFamily: Qt.platform.os === "windows" ? "Consolas" : Qt.platform.os === "osx" ? "Menlo" : "monospace"
    readonly property int compactWidth: 900
    readonly property int pageMargin: 24
    readonly property int spacing: 16
    readonly property int cardRadius: 20
    readonly property int controlHeight: 42
    readonly property int animationDuration: 140
}
