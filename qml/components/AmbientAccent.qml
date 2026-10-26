import QtQuick
import QtQuick.Window
import Flint.Desktop

Item {
    id: accent
    // 0: a breathing contour for Focus. 1: a single selected-card edge sweep.
    property int kind: 0
    property bool active: true
    property bool moving: false
    property real strength: 1.0
    property real cornerRadius: Theme.cardRadius
    property real phase: 0.0
    property real sweep: 1.0
    property bool initialized: false
    readonly property bool gpuAvailable: GraphicsInfo.api !== GraphicsInfo.Software && GraphicsInfo.api !== GraphicsInfo.Unknown
    readonly property bool exposed: active && visible && Window.window !== null
        && Window.window.visible && Window.window.visibility !== Window.Minimized
    readonly property bool canAnimate: exposed && gpuAvailable && Theme.animations && Window.window.active
    readonly property bool motionRunning: canAnimate && kind === 0 && moving
    readonly property bool highlightRunning: sweepAnimation.running
    function highlight() {
        if (!initialized || kind !== 1) return;
        sweepAnimation.stop();
        sweep = 1;
        if (canAnimate) sweepAnimation.restart();
    }
    onExposedChanged: {
        if (!initialized) return;
        if (exposed) highlight();
        else { sweepAnimation.stop(); sweep = 1; }
    }
    onCanAnimateChanged: if (initialized && !canAnimate) { sweepAnimation.stop(); sweep = 1; }
    Component.onCompleted: { initialized = true; highlight(); }
    Timer {
        objectName: "accentClock"
        interval: 33; repeat: true
        running: accent.motionRunning
        onTriggered: accent.phase += 0.033
    }
    NumberAnimation {
        id: sweepAnimation
        target: accent; property: "sweep"
        from: 0; to: 1
        duration: 1100
        easing.type: Easing.InOutCubic
    }
    Loader {
        anchors.fill: parent
        active: accent.exposed && accent.gpuAvailable
        sourceComponent: ShaderEffect {
            property vector2d resolution: Qt.vector2d(width, height)
            property color ink: Theme.accent
            property real kind: accent.kind
            property real phase: accent.phase
            property real sweep: accent.sweep
            property real strength: accent.strength
            property real cornerRadius: accent.cornerRadius
            fragmentShader: "qrc:/shaders/accent.frag.qsb"
        }
    }
    Rectangle {
        visible: accent.exposed && !accent.gpuAvailable
        anchors.centerIn: parent
        width: accent.kind === 1 ? parent.width : Math.min(parent.width, parent.height)*0.65
        height: accent.kind === 1 ? parent.height : width
        radius: accent.kind === 1 ? accent.cornerRadius : width/2
        color: "transparent"
        border.color: Theme.accent
        border.width: 1
        opacity: accent.kind === 1 ? 0.25 : 0.12
    }
}
