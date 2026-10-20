import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Flint.Desktop

AppPage {
    id: page
    positionKey: "faces-" + tab
    required property var service
    required property var studioService
    property int tab: Theme.faceTab
    onTabChanged: Theme.faceTab = tab
    SectionTabs {
        id: faceTabs
        currentIndex: page.tab
        Layout.fillWidth: true
        labels: [qsTr("On watch"), qsTr("My designs"), "Face Studio"]
        onCurrentIndexChanged: page.tab = currentIndex
    }
    Caption {
        visible: page.tab === 0
        text: qsTr("Faces reported by your firmware. Selecting a face requires device confirmation.")
        Layout.fillWidth: true
    }
    AppTextField {
        id: search
        visible: page.tab !== 2
        Layout.fillWidth: true
        placeholderText: qsTr("Search watch faces…")
        Accessible.name: placeholderText
    }
    AppButton { visible: page.tab === 0; text: qsTr("Refresh faces"); available: page.service.supported; onClicked: page.service.refresh() }
    Caption { visible: page.tab === 0 && !page.service.faces.length; text: qsTr("Connect a watch to load its available faces."); Layout.fillWidth: true }
    GridLayout {
        visible: page.tab === 0
        Layout.fillWidth: true
        columns: width >= 750 ? 3 : width >= 480 ? 2 : 1
        columnSpacing: 12
        rowSpacing: 12
        Repeater {
            model: page.service.faces.filter(face => (face.name + " " + face.id).toLowerCase().includes(search.text.toLowerCase()))
            delegate: AppCard {
                required property var modelData
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                implicitHeight: faceContent.implicitHeight + topPadding + bottomPadding
                ColumnLayout {
                    id: faceContent; anchors.fill: parent; spacing: 12
                    Label { text: modelData.name; textFormat: Text.PlainText; font.pixelSize: 22; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                    Caption { text: modelData.author + " · " + modelData.id; Layout.fillWidth: true }
                    Caption { text: qsTr("Screen preview unavailable"); Layout.fillWidth: true }
                    AppButton { text: page.service.values.active_face === modelData.id ? qsTr("Selected") : qsTr("Use on watch"); primary: true; available: page.service.supported && page.service.values.active_face !== modelData.id; onClicked: page.service.selectFace(modelData.id) }
                }
            }
        }
    }
    AppSwitch { id: favorites; visible: page.tab === 1; text: qsTr("Favorites only") }
    Repeater {
        model: page.tab === 1 ? page.studioService.library.map((item,index) => ({item:item,index:index})).filter(entry => (!favorites.checked || entry.item.favorite) && entry.item.design.name.toLowerCase().includes(search.text.toLowerCase())) : []
        delegate: AppCard {
            required property var modelData
            Layout.fillWidth: true
            implicitHeight: designContent.implicitHeight + topPadding + bottomPadding
            ColumnLayout {
                id: designContent
                anchors.fill: parent
                spacing: 10
                Label { text: (modelData.item.favorite ? "★ " : "") + modelData.item.design.name; textFormat: Text.PlainText; font.pixelSize: 22; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                Flow {
                    Layout.fillWidth: true; Layout.preferredHeight: childrenRect.height; spacing: 10
                    AppButton { iconName: "edit"; text: qsTr("Edit design"); onClicked: { page.studioService.load(modelData.index); faceTabs.currentIndex = 2 } }
                    AppButton { text: qsTr("Favorite"); onClicked: page.studioService.favorite(modelData.index) }
                }
            }
        }
    }
    AppButton { visible: page.tab === 1; text: qsTr("Open Face Studio"); primary: true; onClicked: faceTabs.currentIndex = 2 }
    FaceStudioEditor { frameVisible: !page.inSwipeView || page.currentSwipePage; visible: page.tab === 2; service: page.studioService }
}
