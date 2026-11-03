import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import Flint.Desktop

AppPage {
    id: page
    positionKey: "faces-" + tab
    required property var service
    required property var studioService
    readonly property var filteredFaces: service.faces.filter(face => (face.name + " " + face.id).toLowerCase().includes(search.text.toLowerCase()))
    readonly property var filteredDesigns: studioService.library.map((item,index) => ({item:item,index:index})).filter(entry => (!favorites.checked || entry.item.favorite) && entry.item.design.name.toLowerCase().includes(search.text.toLowerCase()))
    property var packages: typeof miniappStore !== "undefined" ? miniappStore : null
    FileDialog { id: packageFile; fileMode: FileDialog.OpenFile; nameFilters: ["Flint Miniapp (*.flintapp)"]; onAccepted: if (page.packages) page.packages.install(selectedFile) }
    property int tab: Theme.faceTab
    onTabChanged: Theme.faceTab = tab
    SectionTabs {
        id: faceTabs
        currentIndex: page.tab
        Layout.fillWidth: true
        labels: [qsTr("On watch"), qsTr("My designs"), "Face Studio", qsTr("Market")]
        onCurrentIndexChanged: page.tab = currentIndex
    }
    RowLayout {
        visible: page.tab < 2
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
                    Caption { text: modelData.author || "FlinT"; textFormat: Text.PlainText; Layout.fillWidth: true }
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
    AppCard {
        visible: page.tab === 3
        Layout.fillWidth: true
        implicitHeight: packageContent.implicitHeight + topPadding + bottomPadding
        ColumnLayout {
            id: packageContent
            anchors.fill: parent
            spacing: 16
            Label { text: qsTr("Watch market"); color: Theme.text; font.pixelSize: 24; Layout.fillWidth: true; wrapMode: Text.WordWrap }
            Caption { text: qsTr("Spark Base is included and protected. Install additional faces, themes and bundles from this local catalog."); Layout.fillWidth: true; wrapMode: Text.WordWrap }
            RowLayout {
                Layout.fillWidth: true
                AppButton { text: qsTr("Import package…"); primary: true; available: page.packages && page.packages.supported && !page.packages.busy; onClicked: packageFile.open() }
                AppButton { text: qsTr("Refresh"); available: page.packages && page.packages.supported && !page.packages.busy; onClicked: page.packages.refresh() }
            }
            AppButton { text: qsTr("Restore default theme"); available: page.packages && page.packages.supported && !page.packages.busy; onClicked: page.packages.useTheme("") }
            Caption { text: page.packages && page.packages.supported ? page.packages.state : qsTr("Connect a watch with Miniapps v1 firmware."); Layout.fillWidth: true; wrapMode: Text.WordWrap }
            Caption {
                text: page.packages && page.packages.storage.partition ?
                    qsTr("Package storage: %1 · %2 KB free / %3 KB total · %4 slots available")
                      .arg(page.packages.storage.partition).arg(Math.floor(page.packages.storage.free / 1024))
                      .arg(Math.floor(page.packages.storage.total / 1024)).arg(page.packages.storage.capacity - page.packages.storage.installed) :
                    qsTr("Refresh to check storage before installing.")
                Layout.fillWidth: true; wrapMode: Text.WordWrap
            }
            Label { text: qsTr("Catalog"); color: Theme.text; font.pixelSize: 20 }
            Repeater {
                model: page.packages ? page.packages.catalog : []
                delegate: RowLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    Label { text: modelData.name; textFormat: Text.PlainText; color: Theme.text; Layout.fillWidth: true }
                    AppButton {
                        text: qsTr("Install / update")
                        available: page.packages && page.packages.canInstall(modelData.id)
                        onClicked: page.packages.installFromCatalog(modelData.id)
                    }
                }
            }
            Label { text: qsTr("On watch"); color: Theme.text; font.pixelSize: 20 }
            Repeater {
                model: page.packages ? page.packages.installed : []
                delegate: ColumnLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: 10
                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }
                    Label { text: modelData.name + " · v" + modelData.version; textFormat: Text.PlainText; color: Theme.text; font.pixelSize: 20; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                    Caption { text: [qsTr("Watch face"), qsTr("Interface theme"), qsTr("Face and theme")][modelData.kind]; Layout.fillWidth: true }
                    Flow {
                        Layout.fillWidth: true
                        spacing: 10
                        AppButton { text: qsTr("Use package"); available: !page.packages.busy; onClicked: page.packages.usePackage(modelData.id) }
                        AppButton { text: modelData["protected"] ? qsTr("Protected") : qsTr("Remove"); available: !modelData["protected"] && !page.packages.busy; onClicked: page.packages.remove(modelData.id) }
                    }
                }
            }
        }
    }
    FaceStudioEditor { frameVisible: page.tab === 2 && (!page.inSwipeView || page.currentSwipePage); visible: page.tab === 2; service: page.studioService }
}
