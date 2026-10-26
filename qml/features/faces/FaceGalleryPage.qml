import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

AppPage {
    id: page
    positionKey: "faces-" + tab
    required property var service
    required property var studioService
    readonly property var filteredFaces: service.faces.filter(face => (face.name + " " + face.id).toLowerCase().includes(search.text.toLowerCase()))
    readonly property var filteredDesigns: studioService.library.map((item,index) => ({item:item,index:index})).filter(entry => (!favorites.checked || entry.item.favorite) && entry.item.design.name.toLowerCase().includes(search.text.toLowerCase()))
    property int tab: Theme.faceTab
    onTabChanged: Theme.faceTab = tab
    SectionTabs {
        id: faceTabs
        currentIndex: page.tab
        Layout.fillWidth: true
        labels: [qsTr("On watch"), qsTr("My designs"), "Face Studio"]
        onCurrentIndexChanged: page.tab = currentIndex
    }
    RowLayout {
        visible: page.tab !== 2
        Layout.fillWidth: true
        spacing: 12
        AppTextField {
            id: search
            Layout.fillWidth: true; Layout.minimumWidth: 0
            placeholderText: qsTr("Search watch faces…")
            Accessible.name: placeholderText
        }
        AppButton { visible: page.tab === 0; iconName: "sync"; text: qsTr("Refresh faces"); available: page.service.supported; onClicked: page.service.refresh() }
    }
    Caption { visible: page.tab === 0; text: qsTr("Faces reported by your firmware. Selecting a face requires device confirmation."); Layout.fillWidth: true }
    Caption { visible: page.tab === 0 && !page.service.faces.length; text: qsTr("Connect a watch to load its available faces."); Layout.fillWidth: true }
    Caption { visible: page.tab === 0 && page.service.faces.length > 0 && !page.filteredFaces.length; text: qsTr("No faces match your search."); Layout.fillWidth: true }
    GridLayout {
        visible: page.tab === 0
        Layout.fillWidth: true
        columns: width >= 960 ? 3 : width >= 570 ? 2 : 1
        columnSpacing: 16; rowSpacing: 16
        Repeater {
            model: page.filteredFaces
            delegate: AppCard {
                required property var modelData
                required property int index
                Layout.fillWidth: true; Layout.minimumWidth: 0
                implicitHeight: faceContent.implicitHeight + topPadding + bottomPadding
                background: Rectangle {
                    color: Theme.surface; radius: Theme.cardRadius
                    AmbientAccent {
                        objectName: "selectedFaceAccent-" + modelData.id
                        anchors.fill: parent
                        kind: 1
                        active: page.tab === 0 && (!page.inSwipeView || page.currentSwipePage)
                            && page.service.values.active_face === modelData.id
                        strength: 0.85
                    }
                }
                ColumnLayout {
                    id: faceContent; anchors.fill: parent; spacing: 14
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: (index + 1).toString().padStart(2, "0"); color: Theme.muted; font.pixelSize: 13; font.family: Theme.monoFamily; Layout.fillWidth: true }
                        AppIcon { name: modelData.id === "studio" ? "edit" : modelData.id === "companion" ? "companion" : "clock"; tint: Theme.accent; Layout.preferredWidth: 32; Layout.preferredHeight: 32 }
                    }
                    Label { text: modelData.name; textFormat: Text.PlainText; font.pixelSize: 24; font.weight: Font.Medium; font.letterSpacing: -0.8; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                    Caption { text: modelData.author; textFormat: Text.PlainText; Layout.fillWidth: true }
                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }
                    Caption { text: qsTr("Screen preview unavailable"); Layout.fillWidth: true }
                    AppButton {
                        text: page.service.values.active_face === modelData.id ? qsTr("Selected") : qsTr("Use on watch")
                        iconName: page.service.values.active_face === modelData.id ? "check" : "right"
                        primary: page.service.values.active_face !== modelData.id
                        available: page.service.supported && page.service.values.active_face !== modelData.id
                        onClicked: page.service.selectFace(modelData.id)
                    }
                }
            }
        }
    }
    AppSwitch { id: favorites; visible: page.tab === 1; text: qsTr("Favorites only") }
    GridLayout {
        visible: page.tab === 1
        Layout.fillWidth: true
        columns: width >= 960 ? 3 : width >= 570 ? 2 : 1
        columnSpacing: 16; rowSpacing: 16
        Repeater {
            model: page.tab === 1 ? page.filteredDesigns : []
            delegate: AppCard {
                required property var modelData
                Layout.fillWidth: true; Layout.minimumWidth: 0
                implicitHeight: designContent.implicitHeight + topPadding + bottomPadding
                ColumnLayout {
                    id: designContent; anchors.fill: parent; spacing: 12
                    Rectangle {
                        Layout.fillWidth: true; implicitHeight: 144
                        color: "#090f18"; radius: 16
                        Image {
                            anchors.fill: parent; anchors.margins: 10
                            source: typeof designPreviewAvailable === "undefined" ? "" : "image://designpreview/" + encodeURIComponent(JSON.stringify(modelData.item.design)) + "/0"
                            fillMode: Image.PreserveAspectFit
                            smooth: false; cache: true
                        }
                    }
                    Caption { text: qsTr("Draft preview · sample time"); Layout.fillWidth: true }
                    Label { text: (modelData.item.favorite ? "★ " : "") + modelData.item.design.name; textFormat: Text.PlainText; font.pixelSize: 24; font.weight: Font.Medium; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                    Flow {
                        Layout.fillWidth: true; Layout.preferredHeight: childrenRect.height; spacing: 8
                        AppButton { iconName: "edit"; text: qsTr("Edit design"); onClicked: { page.studioService.load(modelData.index); faceTabs.currentIndex = 2 } }
                        AppButton { text: qsTr("Favorite"); onClicked: page.studioService.favorite(modelData.index) }
                    }
                }
            }
        }
    }
    Caption { visible: page.tab === 1 && !page.studioService.library.length; text: qsTr("Create your first design in Face Studio."); Layout.fillWidth: true }
    Caption { visible: page.tab === 1 && page.studioService.library.length > 0 && !page.filteredDesigns.length; text: qsTr("No faces match your search."); Layout.fillWidth: true }
    AppButton { visible: page.tab === 1; text: qsTr("Open Face Studio"); iconName: "edit"; primary: true; onClicked: faceTabs.currentIndex = 2 }
    FaceStudioEditor { frameVisible: page.tab === 2 && (!page.inSwipeView || page.currentSwipePage); visible: page.tab === 2; service: page.studioService }
}
