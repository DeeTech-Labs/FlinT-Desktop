import QtQuick
import QtQuick.Window
import Flint.Desktop

Item {
    id: atmosphere
    objectName: "heroAtmosphere"
    property bool active: true
    property real phase: 4.0
    readonly property bool motionRunning: active && visible && Theme.animations
        && Window.window !== null && Window.window.visible
        && Window.window.visibility !== Window.Minimized && Window.window.active
    readonly property bool gpuAvailable: GraphicsInfo.api !== GraphicsInfo.Software && GraphicsInfo.api !== GraphicsInfo.Unknown
    // The clock is capped at 30 Hz and pauses off-page, minimized or without focus.
    Timer {
        objectName: "atmosphereClock"
        interval: 33
        repeat: true
        running: atmosphere.motionRunning && atmosphere.gpuAvailable
        onTriggered: atmosphere.phase += 0.033
    }
    Loader {
        anchors.fill: parent
        active: atmosphere.active && atmosphere.gpuAvailable
        sourceComponent: ShaderEffect {
            property vector2d resolution: Qt.vector2d(width, height)
            property color ink: "#30d4ef"
            property real phase: atmosphere.phase

            fragmentShader: "qrc:/shaders/atmosphere.frag.qsb"
        }
    }
    // A composed static alternative for software rendering; no shader warnings or blank hero.
    Rectangle {
        visible: !atmosphere.gpuAvailable
        anchors.right: parent.right; anchors.rightMargin: 40; anchors.verticalCenter: parent.verticalCenter
        width: parent.width * 0.42; height: parent.height * 0.48; radius: height / 2
        rotation: -23
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0; color: "#08233e" }
            GradientStop { position: 0.55; color: "#0c5480" }
            GradientStop { position: 1; color: "#208b9c" }
        }
        opacity: 0.6
    }
}
