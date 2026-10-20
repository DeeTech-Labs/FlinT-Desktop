import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

ScrollView {
    id: page
    readonly property bool inSwipeView: SwipeView.view !== null && SwipeView.view !== undefined
    readonly property bool currentSwipePage: SwipeView.isCurrentItem
    property string positionKey: "default"
    property var scrollPositions: ({})
    property bool restoringPosition: false
    onPositionKeyChanged: {
        restoringPosition = true;
        Qt.callLater(() => {
            if (page.contentItem && page.contentItem.contentY !== undefined)
                page.contentItem.contentY = Math.max(0,Math.min(page.scrollPositions[page.positionKey] || 0,page.contentItem.contentHeight - page.availableHeight));
            page.restoringPosition = false;
        });
    }
    Connections {
        target: page.contentItem
        ignoreUnknownSignals: true
        function onContentYChanged() {
            if (!page.restoringPosition && page.contentItem && page.contentItem.contentY !== undefined)
                page.scrollPositions[page.positionKey] = page.contentItem.contentY;
        }
    }
    Layout.minimumWidth: 0
    Layout.maximumWidth: parent ? parent.width : Infinity
    default property alias pageContent: body.data
    contentWidth: availableWidth
    clip: true
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    Component.onCompleted: {
        if (contentItem && contentItem.flickableDirection !== undefined) {
            contentItem.flickableDirection = Flickable.VerticalFlick;
            contentItem.boundsBehavior = Flickable.StopAtBounds;
            contentItem.interactive = true;
        }
    }
    contentData: ColumnLayout {
        id: body
        width: page.availableWidth
        spacing: Theme.spacing
    }
}
