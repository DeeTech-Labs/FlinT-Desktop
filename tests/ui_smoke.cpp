#include "device/DeviceSession.h"
#include "services/WatchSettings.h"
#include "services/FaceStudio.h"
#include "services/CompanionTools.h"
#include "localization/Localization.h"

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>
#ifdef Q_OS_LINUX
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>
#include <QSocketNotifier>
#endif

class UiSmoke : public QObject {
    Q_OBJECT
private slots:
    void hardwareBleRoundTrip()
    {
        const auto address = qEnvironmentVariable("FLINT_HARDWARE_BLE_ADDR").toUpper();
        if (address.isEmpty()) QSKIP("Set FLINT_HARDWARE_BLE_ADDR to run the opt-in real-device check.");
        QVERIFY(QRegularExpression("^[0-9A-F]{2}(:[0-9A-F]{2}){5}$").match(address).hasMatch());
        DeviceSession session(nullptr,[] { return QVariantList{}; });
        session.setAutoSyncTime(false);
        bool selected = false;
        connect(&session,&DeviceSession::portsChanged,&session,[&] {
            if (selected) return;
            for (const auto& entry : session.bleDevices()) {
                if (entry.toMap().value("path").toString().toUpper() != address) continue;
                selected = true;
                session.connectBle(entry.toMap().value("path").toString());
                break;
            }
        });
        QSignalSpy samples(&session,&DeviceSession::sampleReceived);
        QSignalSpy replies(&session,&DeviceSession::requestFinished);
        session.scanBle();
        QTRY_VERIFY_WITH_TIMEOUT(session.connected(),45000);
        QTRY_VERIFY_WITH_TIMEOUT(!samples.isEmpty(),10000);
        QCOMPARE(session.transport(),QStringLiteral("BLE"));
        QCOMPARE(session.info().value("protocol").toInt(),1);
        QTRY_VERIFY(!session.busy());
        const auto settingsId = session.request("SETTINGS");
        QVERIFY(settingsId > 0);
        QTRY_VERIFY_WITH_TIMEOUT(!replies.isEmpty(),10000);
        auto reply = replies.takeFirst();
        QCOMPARE(reply[0].toInt(),settingsId);
        QVERIFY2(reply[1].toBool(),qPrintable(reply[3].toString()));
        QVERIFY(!reply[2].toMap().value("timezone").toString().isEmpty());
        QTRY_VERIFY(!session.busy());
        const auto deniedId = session.request("APPS");
        QVERIFY(deniedId > 0);
        QTRY_VERIFY_WITH_TIMEOUT(!replies.isEmpty(),10000);
        reply = replies.takeFirst();
        QCOMPARE(reply[0].toInt(),deniedId);
        QVERIFY(!reply[1].toBool());
        QCOMPARE(reply[3].toString(),QStringLiteral("authenticated_transport_required"));
        QTRY_VERIFY(!session.busy());
        session.syncTime();
        QTRY_VERIFY_WITH_TIMEOUT(!session.lastSync().isEmpty(),10000);
        session.disconnectDevice();
        QVERIFY(!session.connected());
        samples.clear();
        session.connectBle(address);
        QTRY_VERIFY_WITH_TIMEOUT(session.connected(),45000);
        QTRY_VERIFY_WITH_TIMEOUT(!samples.isEmpty(),10000);
        session.disconnectDevice();
        QVERIFY(!session.connected());
    }
    void bleDiscoveryCandidates()
    {
        QBluetoothDeviceInfo device(QBluetoothAddress("1C:DB:D4:C6:49:16"),"FlinT Spark",0);
        device.setCoreConfigurations(QBluetoothDeviceInfo::LowEnergyCoreConfiguration);
        QVERIFY(BleTransport::isCandidate(device)); // Scan response arrived before UUIDs.
        device.setName("Other device");
        QVERIFY(!BleTransport::isCandidate(device));
        device.setServiceUuids({QBluetoothUuid("e7f10001-7a1e-4b5c-9d2f-46544c494e54")});
        QVERIFY(BleTransport::isCandidate(device));
        device.setCoreConfigurations(QBluetoothDeviceInfo::BaseRateCoreConfiguration);
        QVERIFY(!BleTransport::isCandidate(device));
        device.setCoreConfigurations(QBluetoothDeviceInfo::LowEnergyCoreConfiguration);
        device.setName("FlinT Spark");
        device.setServiceUuids({QBluetoothUuid("0000180f-0000-1000-8000-00805f9b34fb")});
        QVERIFY(!BleTransport::isCandidate(device)); // A known different service is not FlinT.
    }
    void companionNotesAndFocus()
    {
        DeviceSession session;
        WatchSettings settings(&session);
        CompanionTools tools(&session);
        session.startSimulator();
        QTRY_VERIFY(tools.supported() && !settings.faces().isEmpty() && !session.busy());
        QTest::qWait(2500); // Allow capability-driven initialization to finish.
        tools.sendNote(QString::fromUtf8("Заметка"));
        QTRY_COMPARE(tools.note(),QString::fromUtf8("Заметка"));
        QTRY_VERIFY(!session.busy());
        tools.startFocus(1);
        QTRY_VERIFY(tools.focusRemaining() > 0 && tools.focusRemaining() <= 60);
        QTRY_VERIFY(!session.busy());
        tools.stopFocus();
        QTRY_COMPARE(tools.focusRemaining(),0);
        QVERIFY(!tools.timeline().isEmpty());
        QTemporaryDir directory;
        QVERIFY(tools.exportTimeline(QUrl::fromLocalFile(directory.filePath("timeline.json"))));
        session.disconnectDevice();
    }
    void studioImportExportAndInstall()
    {
        DeviceSession session;
        WatchSettings settings(&session);
        FaceStudio studio(&session);
        session.startSimulator();
        QTRY_VERIFY(studio.supported() && !settings.faces().isEmpty() && !session.busy());
        studio.paint(0,true); studio.paint(255,true);
        QVERIFY(studio.design().value("pixels").toString().startsWith('8'));
        QVERIFY(studio.design().value("pixels").toString().endsWith('1'));
        studio.edit("background","#123456");
        studio.edit("background","invalid");
        QCOMPARE(studio.design().value("background").toString(),"#123456");
        studio.edit("timeY",128.5);
        QCOMPARE(studio.design().value("timeY").toInt(),128);
        QTemporaryDir directory;
        const auto path = QUrl::fromLocalFile(directory.filePath("test.flintface"));
        QVERIFY(studio.exportDesign(path));
        studio.paint(0,false);
        QVERIFY(studio.importDesign(path));
        QVERIFY(studio.design().value("pixels").toString().startsWith('8'));
        studio.save();
        studio.install();
        QTRY_COMPARE(studio.state(),QString("Installed and selected"));
        session.refreshStatus();
        QTRY_COMPARE(session.status().value("active_face").toString(),QString("studio"));
        studio.setLiveReload(true);
        auto modified = studio.design(); modified["background"] = "#abcdef";
        QFile updated(path.toLocalFile());
        QVERIFY(updated.open(QIODevice::WriteOnly | QIODevice::Truncate));
        updated.write(QJsonDocument(QJsonObject::fromVariantMap(modified)).toJson()); updated.close();
        QTRY_COMPARE(studio.design().value("background").toString(),QString("#abcdef"));
        QTRY_COMPARE(studio.state(),QString("Installed and selected"));
        studio.setLiveReload(false);
        session.disconnectDevice();
    }
    void simulatorSettingsProfilesAndBackup()
    {
        DeviceSession session;
        WatchSettings settings(&session);
        session.startSimulator();
        QTRY_VERIFY(session.connected() && settings.supported() && !settings.faces().isEmpty() && !settings.timezones().isEmpty());
        QCOMPARE(session.transport(), "SIMULATOR");
        QCOMPARE(settings.values().value("timezone").toString(), "Europe/Moscow");
        settings.edit("show_seconds",1);
        settings.edit("timezone","UTC");
        settings.apply();
        QTRY_COMPARE(settings.values().value("timezone").toString(), "UTC");
        QCOMPARE(settings.values().value("show_seconds").toInt(),1);
        QVERIFY(!settings.dirty());
        QTemporaryDir directory;
        const auto backup = QUrl::fromLocalFile(directory.filePath("backup.json"));
        QVERIFY(settings.backup(backup));
        settings.saveProfile("Roundtrip");
        const int profile = settings.profiles().size() - 1;
        settings.edit("hour_format",1);
        settings.apply();
        QTRY_COMPARE(settings.values().value("hour_format").toInt(),1);
        QVERIFY(settings.restore(backup));
        QVERIFY(settings.dirty());
        settings.apply();
        QTRY_COMPARE(settings.values().value("hour_format").toInt(),0);
        settings.edit("timezone","invalid/timezone");
        settings.apply();
        QCOMPARE(settings.state(),QString("Invalid settings"));
        QCOMPARE(settings.values().value("timezone").toString(),"UTC");
        settings.loadProfile(profile);
        QCOMPARE(settings.draft().value("timezone").toString(),"UTC");
        settings.discard();
        QSignalSpy responses(&session,&DeviceSession::requestFinished);
        QTRY_VERIFY(!session.busy());
        QVERIFY(session.request("UNKNOWN") > 0);
        QTRY_COMPARE(responses.size(),1);
        QVERIFY(!responses.first().at(1).toBool());
        QCOMPARE(responses.first().at(3).toString(),"unsupported_operation");
        QVERIFY(session.connected());
        session.disconnectDevice();
        QVERIFY(!session.connected());
    }
    void discoveryRetriesAndManualPause()
    {
        QVariantList ports;
        DeviceSession session(nullptr, [&] { return ports; });
        QSignalSpy failures(&session, &DeviceSession::failed);
        session.setAutoConnect(true);
        QTest::qWait(50);
        QCOMPARE(failures.size(), 0);
        ports = {QVariantMap{{"path", "/missing-flint-a"}, {"automatic", true}},
                 QVariantMap{{"path", "/missing-flint-b"}, {"automatic", true}},
                 QVariantMap{{"path", "/unrelated-adapter"}, {"automatic", false}}};
        QTRY_COMPARE_WITH_TIMEOUT(failures.size(), 2, 3500);
        QTest::qWait(1100);
        QCOMPARE(failures.size(), 2); // Backoff, and unrelated ports are not opened.
        session.disconnectDevice();
        QVERIFY(!session.autoConnect());
        ports = {};
        QTest::qWait(1100);
        QCOMPARE(failures.size(), 2);
        session.setAutoConnect(true);
        QTest::qWait(50);
        ports = {QVariantMap{{"path", "/missing-flint-a"}, {"automatic", true}}};
        QTRY_COMPARE_WITH_TIMEOUT(failures.size(), 3, 2000);
    }
#ifdef Q_OS_LINUX
    void discoveryHandshakeAndReconnect()
    {
        const int master = posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK);
        QVERIFY(master >= 0);
        QVERIFY(grantpt(master) == 0 && unlockpt(master) == 0);
        const auto path = QString::fromLocal8Bit(ptsname(master));
        QByteArray incoming;
        int hellos = 0;
        QSocketNotifier notifier(master, QSocketNotifier::Read);
        connect(&notifier, &QSocketNotifier::activated, this, [&] {
            char bytes[4096];
            const auto count = ::read(master, bytes, sizeof(bytes));
            if (count <= 0) return;
            incoming.append(bytes, count);
            while (incoming.contains('\n')) {
                const auto command = incoming.left(incoming.indexOf('\n')).trimmed();
                incoming.remove(0, incoming.indexOf('\n') + 1);
                QByteArray response;
                if (command == "@HELLO") {
                    ++hellos;
                    response = "FLINT HELLO 1 flint_spark\n";
                } else if (command == "@INFO") {
                    response = R"(FLINT INFO {"protocol":1,"id":"test","model":"Spark","board":"spark","chip":"esp32c3","firmware":"dev","capabilities":["info","status"]})";
                    response += '\n';
                } else if (command == "@STATUS") {
                    response = R"(FLINT STATUS {"time_valid":false,"epoch":null,"pc_synced":false,"usb_connected":true,"uptime_ms":1,"free_heap_bytes":100,"timezone":"UTC","active_face":"digital","settings":{}})";
                    response += '\n';
                }
                if (!response.isEmpty()) ::write(master, response.constData(), response.size());
            }
        });
        QVariantList ports = {QVariantMap{{"path", path}, {"automatic", true}}};
        {
            DeviceSession session(nullptr, [&] { return ports; });
            QSignalSpy samples(&session, &DeviceSession::sampleReceived);
            session.setAutoConnect(true);
            QTRY_VERIFY(session.connected() && samples.size() >= 1);
            QCOMPARE(hellos, 1);
            ports = {}; // USB removal, even before the next status poll.
            QTRY_VERIFY(!session.connected());
            ports = {QVariantMap{{"path", path}, {"automatic", true}}};
            QTRY_VERIFY(session.connected() && hellos == 2);
            session.disconnectDevice();
            QTest::qWait(1100);
            QCOMPARE(hellos, 2);
            QVERIFY(!session.connected());
        }
        ::close(master);
    }
#endif
    void timezones()
    {
        DeviceSession session;
        QCOMPARE(session.formatTime(1704148200, "Europe/Moscow", "yyyy-MM-dd HH:mm"), "2024-01-02 01:30");
        QCOMPARE(session.formatTime(1719873000, "Europe/Berlin", "HH:mm"), "00:30");
    }
    void customPagePackAndFallback()
    {
        const auto root = Localization::userDirectory();
        QVERIFY(QDir().mkpath(root));
        QTemporaryDir pack(root + "/test-XXXXXX");
        QVERIFY(pack.isValid());
        const auto code = QDir(pack.path()).dirName();
        QFile metadata(pack.filePath("meta.json"));
        QVERIFY(metadata.open(QIODevice::WriteOnly));
        metadata.write(QJsonDocument(QJsonObject { { "language", code }, { "name", "Test language" } }).toJson());
        metadata.close();
        QFile overview(pack.filePath("overview.json"));
        QVERIFY(overview.open(QIODevice::WriteOnly));
        overview.write(R"({"contexts":{"AppNavigation":{"Overview":"Custom overview"}}})");
        overview.close();
        Localization localization;
        localization.setLanguage(code);
        QCOMPARE(QCoreApplication::translate("AppNavigation", "Overview"), "Custom overview");
        QCOMPARE(QCoreApplication::translate("DeviceSession", "Connected"), "Connected");
        localization.setLanguage("not-installed");
        QCOMPARE(localization.language(), "en");
    }
    void responsiveAndLocalized()
    {
        Localization localization;
        DeviceSession session;
        WatchSettings watchSettings(&session);
        FaceStudio faceStudio(&session);
        CompanionTools companionTools(&session);
        QStringList warnings;
        QQmlApplicationEngine engine;
        connect(&engine, &QQmlEngine::warnings, this, [&](const QList<QQmlError>& errors) {
            for (const auto& error : errors)
                warnings.append(error.toString());
        });
        engine.rootContext()->setContextProperty("companionTools", &companionTools);
        engine.rootContext()->setContextProperty("faceStudio", &faceStudio);
        engine.rootContext()->setContextProperty("watchSettings", &watchSettings);
        engine.rootContext()->setContextProperty("device", &session);
        engine.rootContext()->setContextProperty("localization", &localization);
        connect(&localization, &Localization::languageChanged, &engine, &QQmlEngine::retranslate);
        connect(&localization, &Localization::languageChanged, &session, &DeviceSession::stateChanged);
        engine.load(QUrl("qrc:/qt/qml/Flint/Desktop/Main.qml"));
        QVERIFY2(!engine.rootObjects().isEmpty(), qPrintable(warnings.join('\n')));
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        QVERIFY(window);
        auto* selector = window->findChild<QObject*>("languageSelector");
        QVERIFY(selector);
        QCOMPARE(localization.languages().size(), 2);
        QQmlComponent fixture(&engine);
        fixture.setData(R"(
            import QtQuick
            QtObject {
                signal dataChanged()
                signal stateChanged()
                property bool connecting: false
                property string transport: "BLE"
                property var bleDevices: []
                property bool bleScanning: false
                property bool connected: true
                property bool busy: false
                property bool polling: true
                property string connectionState: "Connected"
                property string error: ""
                property string notification: ""
                property int sampleCount: 42
                property string lastUpdated: "2026-10-17T19:30:00Z"
                property string log: "USB connected\n> @STATUS\nFLINT STATUS {...}"
                property bool autoSyncTime: false
                property string lastSync: ""
                property int clockDifference: 0
                property bool autoConnect: true
                property var ports: []
                property var info: ({model: "FlinT Clock", id: "flint-spark-test", board: "Spark", chip: "ESP32-S3", firmware: "1.0", capabilities: ["info", "status", "time_sync"]})
                property var status: ({time_valid: true, epoch: 1704148200, timezone: "Europe/Moscow", uptime_ms: 5000000000, free_heap_bytes: 180000, active_face: "digital", pc_synced: true, settings: {language: 0, hour_format: 0, show_seconds: 255}})
                function formatTime(epoch, zone, format) { return device.formatTime(epoch, zone, format) }
                function refreshPorts() {}
                function connectPort(port) {}
                function disconnectDevice() {}
                function syncTime() {}
                function refreshStatus() {}
                function clearLog() {}
                function saveData(destination) {}
            }
        )",
            QUrl());
        QScopedPointer<QObject> populated(fixture.create());
        QVERIFY2(populated, qPrintable(fixture.errorString()));
        auto* themeSelector = window->findChild<QObject*>("themeSelector");
        QVERIFY(themeSelector);
        for (const auto& language : { QStringLiteral("ru"), QStringLiteral("en") }) {
            localization.setLanguage(language);
            QTest::qWait(1);
            QCOMPARE(selector->property("currentValue").toString(), language);
            QCOMPARE(selector->property("currentText").toString(), language == "ru" ? "Русский" : "English");
            QCOMPARE(QCoreApplication::translate("AppNavigation", "Overview"), language == "ru" ? "Обзор" : "Overview");
            QCOMPARE(QCoreApplication::translate("DeviceSession", "Connected"), language == "ru" ? "Подключено" : "Connected");
            for (int theme : {0, 1, 2}) {
            themeSelector->setProperty("currentIndex", theme);
            QVERIFY(QMetaObject::invokeMethod(themeSelector, "activated", Q_ARG(int, theme)));
            for (const QSize size : { QSize(540, 640), QSize(760, 700), QSize(1024, 768), QSize(1440, 900) }) {
                window->resize(size);
                // Test both the empty state and populated telemetry at every breakpoint.
                for (bool hasData : { false, true }) {
                    window->setProperty("session", QVariant::fromValue(hasData ? populated.data() : &session));
                    for (int page = 0; page < 6; ++page) {
                        window->setProperty("section", page);
                        QTest::qWait(80);
                        QCOMPARE(window->property("compact").toBool(), size.width() < 900);
                        QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join('\n')));
                        const auto overflow = [&](auto&& self, QQuickItem* item) -> QString {
                            if (!item->isVisible())
                                return {};
                            // Ignore vertically scrollable content; horizontal overflow is never intended.
                            if (item->metaObject()->indexOfProperty("text") >= 0 && item->width() > 0) {
                                const auto position = item->mapToItem(window->contentItem(), QPointF());
                                if (position.x() < -1 || position.x() + item->width() > window->width() + 1)
                                    return QString("Overflow: %1 at %2, width %3, viewport %4")
                                            .arg(item->property("text").toString())
                                            .arg(position.x()).arg(item->width()).arg(window->width());
                            }
                            for (auto* child : item->childItems()) {
                                const auto result = self(self, child);
                                if (!result.isEmpty()) return result;
                            }
                            return {};
                        };
                        // Font discovery and layout polish can finish after the first frame,
                        // especially on macOS. Check the settled geometry at every breakpoint.
                        QString overflowMessage;
                        QTRY_VERIFY2_WITH_TIMEOUT((overflowMessage = overflow(overflow, window->contentItem())).isEmpty(),
                                                  qPrintable(overflowMessage), 2000);
                        const auto output = qEnvironmentVariable("FLINT_SCREENSHOT_DIR");
                        if (!output.isEmpty())
                            window->grabWindow().save(QString("%1/%2-%3-%4-%5-theme%6.png").arg(output, language).arg(size.width()).arg(page).arg(hasData ? "connected" : "empty").arg(theme));
                    }
                }
            }
        }
        }
        window->setProperty("session", QVariant::fromValue(&session));
    }
};

int main(int argc, char** argv)
{
    QQuickStyle::setStyle("Basic");
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName("FlintTests");
    QCoreApplication::setApplicationName("FlintUiTests");
    QStandardPaths::setTestModeEnabled(true);
    QTemporaryDir settings;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
    UiSmoke tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "ui_smoke.moc"
