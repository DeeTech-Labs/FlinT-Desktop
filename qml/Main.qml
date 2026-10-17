import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: window
    width: 1160
    height: 820
    minimumWidth: 760
    minimumHeight: 620
    visible: true
    title: Qt.application.name

    function value(object, key) {
        return object[key] === undefined ? "—" : String(object[key])
    }
    function clockValue() {
        if (!device.status.time_valid) return qsTr("Время не установлено")
        return new Date(device.status.epoch * 1000).toISOString().replace("T", " ").replace(".000Z", " UTC")
    }
    function uptime() {
        if (device.status.uptime_ms === undefined) return "—"
        let seconds = Math.floor(device.status.uptime_ms / 1000)
        return Math.floor(seconds / 3600) + " ч " + Math.floor(seconds % 3600 / 60) + " мин " + seconds % 60 + " с"
    }
    function capabilities() {
        const names = { info: qsTr("сведения об устройстве"), status: qsTr("сбор состояния"), time_sync: qsTr("синхронизация времени") }
        return (device.info.capabilities || []).filter(key => names[key] !== undefined).map(key => names[key]).join(", ")
    }
    function setting(name, labels) {
        const settings = device.status.settings || {}
        return settings[name] === undefined ? "—" : labels[settings[name]] || qsTr("Другой")
    }

    FileDialog {
        id: exportDialog
        title: qsTr("Сохранить собранные данные")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Данные JSON (*.json)")]
        defaultSuffix: "json"
        onAccepted: device.saveData(selectedFile)
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            Label { text: "FlinT Desktop"; font.pixelSize: 22; font.bold: true }
            Item { Layout.fillWidth: true }
            BusyIndicator { running: device.busy; implicitWidth: 28; implicitHeight: 28 }
            Label { text: device.connectionState; textFormat: Text.PlainText }
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        ColumnLayout {
            width: parent.width
            spacing: 16

            Pane {
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    RowLayout {
                        Layout.fillWidth: true
                        ComboBox {
                            id: ports
                            Layout.fillWidth: true
                            model: device.ports
                            textRole: "label"
                            valueRole: "path"
                            enabled: !device.connected && !device.busy
                            displayText: count ? currentText : qsTr("USB-устройства не найдены")
                        }
                        Button {
                            text: qsTr("Обновить")
                            enabled: !device.connected && !device.busy
                            onClicked: device.refreshPorts()
                        }
                        Button {
                            text: device.connected || device.busy ? qsTr("Отключить") : qsTr("Подключить")
                            enabled: device.connected || device.busy || ports.count > 0
                            onClicked: {
                                if (device.connected || device.busy) device.disconnectDevice()
                                else device.connectPort(ports.currentValue)
                            }
                        }
                    }
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Подключите часы по USB и выберите порт. Другой монитор порта или помощник синхронизации должен быть закрыт.")
                        wrapMode: Text.WordWrap
                    }
                    Label {
                        Layout.fillWidth: true
                        visible: device.error.length > 0
                        text: device.error
                        textFormat: Text.PlainText
                        wrapMode: Text.WordWrap
                        font.bold: true
                    }
                    Label {
                        Layout.fillWidth: true
                        visible: device.notification.length > 0
                        text: device.notification
                        textFormat: Text.PlainText
                        wrapMode: Text.WordWrap
                    }
                }
            }

            Pane {
                Layout.fillWidth: true
                visible: device.info.id !== undefined
                ColumnLayout {
                    anchors.fill: parent
                    Label {
                        text: window.value(device.info, "model")
                        textFormat: Text.PlainText
                        font.pixelSize: 24
                        font.bold: true
                    }
                    Label {
                        Layout.fillWidth: true
                        text: "ID: " + window.value(device.info, "id") + "   ·   " + window.value(device.info, "board")
                        textFormat: Text.PlainText
                        wrapMode: Text.WordWrap
                    }
                    Label {
                        text: "FlinT OS " + window.value(device.info, "firmware") + "   ·   " + window.value(device.info, "chip")
                        textFormat: Text.PlainText
                    }
                    Label {
                        text: qsTr("Возможности: ") + window.capabilities()
                        textFormat: Text.PlainText
                    }
                }
            }

            Pane {
                Layout.fillWidth: true
                visible: device.sampleCount > 0
                ColumnLayout {
                    anchors.fill: parent
                    Label { text: qsTr("Состояние часов"); font.pixelSize: 20; font.bold: true }
                    Label {
                        text: device.connected ? qsTr("Обновление каждые 2 секунды") : qsTr("Устройство отключено. Показаны последние полученные данные.")
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    GridLayout {
                        columns: 2
                        columnSpacing: 28
                        rowSpacing: 10
                        Label { text: qsTr("Время устройства (UTC)") }
                        Label { text: window.clockValue(); textFormat: Text.PlainText }
                        Label { text: qsTr("Часовой пояс экрана") }
                        Label { text: window.value(device.status, "timezone"); textFormat: Text.PlainText }
                        Label { text: qsTr("Синхронизация с ПК") }
                        Label { text: device.status.pc_synced ? qsTr("Выполнена") : qsTr("Не выполнена после запуска") }
                        Label { text: qsTr("Время работы") }
                        Label { text: window.uptime() }
                        Label { text: qsTr("Свободная память") }
                        Label { text: device.status.free_heap_bytes === undefined ? "—" : Math.round(device.status.free_heap_bytes / 1024) + " КиБ" }
                        Label { text: qsTr("Циферблат") }
                        Label { text: window.value(device.status, "active_face"); textFormat: Text.PlainText }
                        Label { text: qsTr("Язык часов") }
                        Label { text: window.setting("language", {0: qsTr("Русский"), 1: qsTr("English")}) }
                        Label { text: qsTr("Формат времени") }
                        Label { text: window.setting("hour_format", {0: qsTr("24 часа"), 1: qsTr("12 часов")}) }
                        Label { text: qsTr("Показ секунд") }
                        Label { text: window.setting("show_seconds", {0: qsTr("Выключен"), 1: qsTr("Включён"), 255: qsTr("Автоматически")}) }
                        Label { text: qsTr("Получено (UTC)") }
                        Label { text: device.lastUpdated; textFormat: Text.PlainText }
                    }
                    RowLayout {
                        Button {
                            text: qsTr("Синхронизировать время")
                            enabled: device.connected && !device.busy && (device.info.capabilities || []).indexOf("time_sync") >= 0
                            onClicked: device.syncTime()
                        }
                        Button { text: qsTr("Запросить сейчас"); enabled: device.connected && !device.busy; onClicked: device.refreshStatus() }
                        Button { text: qsTr("Сохранить JSON…"); onClicked: exportDialog.open() }
                    }
                    Label { text: qsTr("Снимков в памяти: ") + device.sampleCount + qsTr(" / 600. В файл данные записываются только по кнопке сохранения."); wrapMode: Text.WordWrap; Layout.fillWidth: true }
                }
            }

            Pane {
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    RowLayout {
                        Label { text: qsTr("Журнал USB"); font.pixelSize: 18; font.bold: true; Layout.fillWidth: true }
                        Button { text: qsTr("Очистить"); onClicked: device.clearLog() }
                    }
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 180
                        TextArea {
                            text: device.log
                            readOnly: true
                            selectByMouse: true
                            textFormat: TextEdit.PlainText
                            wrapMode: TextEdit.WrapAnywhere
                            font.family: "monospace"
                            onTextChanged: cursorPosition = length
                        }
                    }
                }
            }
        }
    }
}
