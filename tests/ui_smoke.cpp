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
#include "watch_render/WatchScreen.h"
#include "watch_render/shared/builders.h"
#include <QTemporaryDir>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QWheelEvent>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QScopeGuard>
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
    void screenSceneRoundTrip()
    {
        DeviceSession session;
        WatchScreen screen(&session);
        session.startSimulator();
        QTRY_VERIFY(screen.supported());
        screen.setActive(true);
        QTRY_VERIFY_WITH_TIMEOUT(screen.available(),5000);
        QCOMPARE(screen.width(),280); QCOMPARE(screen.height(),240);
        QVERIFY(screen.sceneBytes()>0 && screen.sceneBytes()<2000);
        WatchImageProvider provider(screen.store()); QSize size;
        const auto frame=provider.requestImage("",&size,{});
        QCOMPARE(size,QSize(280,240)); QVERIFY(!frame.isNull());
        QVERIFY(screen.sendKey("settings")); QVERIFY(screen.sendKey("next")); QVERIFY(screen.sendKey("back"));
        QVERIFY(!screen.sendKey("arbitrary"));
        QTest::qWait(400);
        QTemporaryDir directory;
        QSignalSpy finished(&screen,&WatchScreen::captureFinished);
        QVERIFY(screen.capture(QUrl::fromLocalFile(directory.filePath("actual.png"))));
        QTRY_VERIFY_WITH_TIMEOUT(!finished.isEmpty(),25000);
        QVERIFY2(finished.last().at(0).toBool(),qPrintable(screen.verification()));
        QVERIFY(QFile::exists(directory.filePath("actual.png")));
        QTRY_VERIFY(!screen.capturing());
        // Starting during a metadata request queues safely; cancelling also releases a started capture.
        screen.refresh();
        QVERIFY(screen.capture(QUrl::fromLocalFile(directory.filePath("cancelled.png"))));
        screen.cancelCapture();
        QTRY_VERIFY(!screen.capturing());
        screen.setActive(false);
        QTest::qWait(500);
        const auto transferred=screen.sceneBytes();
        QTest::qWait(1200); QCOMPARE(screen.sceneBytes(),transferred);
        session.disconnectDevice(); QVERIFY(screen.stale());
        session.startSimulator(); screen.setActive(true);
        QTRY_VERIFY_WITH_TIMEOUT(screen.supported() && !screen.stale(),5000);
        session.disconnectDevice();
    }
    void hardwareScreenRoundTrip()
    {
        const auto port=qEnvironmentVariable("FLINT_SCREEN_USB_PORT");
        if (port.isEmpty()) QSKIP("Set FLINT_SCREEN_USB_PORT for actual framebuffer comparison.");
        DeviceSession session; session.setAutoSyncTime(false);
        WatchScreen screen(&session); session.connectPort(port);
        QTRY_VERIFY_WITH_TIMEOUT(screen.supported(),10000);
        QDir().mkpath("/tmp/flint-real-render");
        const QList<QPair<QString,QString>> screens{{"FACE","digital"},{"KEY","settings"},{"KEY","next"},{"KEY","back"},{"FACE","companion"},{"FACE","studio"}};
        int index=0;
        for (const auto& command:screens) {
            screen.setActive(false); QTRY_VERIFY(!session.busy());
            QSignalSpy reply(&session,&DeviceSession::requestFinished);
            QVERIFY(session.request(command.first,command.second));
            QTRY_VERIFY_WITH_TIMEOUT(!reply.isEmpty(),5000);
            QVERIFY2(reply.last().at(1).toBool(),qPrintable(reply.last().at(3).toString()));
            QTest::qWait(300);
            QSignalSpy finished(&screen,&WatchScreen::captureFinished);
            QVERIFY(screen.capture(QUrl::fromLocalFile(QString("/tmp/flint-real-render/frame-%1.png").arg(index++))));
            QTRY_VERIFY2_WITH_TIMEOUT(!finished.isEmpty(),qPrintable(screen.state()+session.error()),25000);
            QVERIFY2(finished.last().at(0).toBool(),qPrintable(screen.verification()+" · "+session.error()+" · "+session.log()));
            QTRY_VERIFY(!screen.capturing());
        }
        QTRY_VERIFY(!session.busy()); session.request("FACE","digital");
        QTRY_VERIFY(!session.busy());session.disconnectDevice();
    }
    void hardwareBleRoundTrip()
    {
        const auto address = qEnvironmentVariable("FLINT_HARDWARE_BLE_ADDR").toUpper();
        if (address.isEmpty()) QSKIP("Set FLINT_HARDWARE_BLE_ADDR to run the opt-in real-device check.");
        QVERIFY(QRegularExpression("^[0-9A-F]{2}(:[0-9A-F]{2}){5}$").match(address).hasMatch());
        DeviceSession session(nullptr,[] { return QVariantList{}; });
        session.setAutoSyncTime(false);
        QSerialPort serial;
        QByteArray serialInput;
        const auto serialPath = qEnvironmentVariable("FLINT_HARDWARE_USB_PORT");
        bool pairingOpened = serialPath.isEmpty();
        QString serialFailure;
        if (!serialPath.isEmpty()) {
            serial.setPortName(serialPath); serial.setBaudRate(115200);
            QVERIFY(serial.open(QIODevice::ReadWrite));
            connect(&serial, &QSerialPort::readyRead, this, [&] {
                serialInput += serial.readAll();
                while (serialInput.contains('\n')) {
                    const auto end = serialInput.indexOf('\n');
                    const auto line = serialInput.left(end).trimmed(); serialInput.remove(0,end + 1);
                    if (!line.startsWith("FLINT RESULT ")) continue;
                    const auto reply = QJsonDocument::fromJson(line.mid(13)).object();
                    const int id = reply.value("id").toInt();
                    if (id == 900) pairingOpened = reply.value("ok").toBool();
                    if (id == 901) {
                        const auto code = reply.value("result").toObject().value("code").toInt(-1);
                        if (code < 0 || QString("%1").arg(code,6,10,QLatin1Char('0')) != session.pairingCode()) {
                            serialFailure = "Serial and Desktop pairing codes differ"; session.confirmPairing(false); continue;
                        }
                        serial.write("@REQ 902 PAIR confirm=" + session.pairingCode().toLatin1() + "\n");
                    }
                    if (id == 902) {
                        if (!reply.value("ok").toBool()) serialFailure = "Serial confirmation failed";
                        session.confirmPairing(reply.value("ok").toBool());
                    }
                }
            });
            serial.write("@REQ 900 PAIR open\n");
            QTRY_VERIFY_WITH_TIMEOUT(pairingOpened, 5000);
            connect(&session, &DeviceSession::pairingCodeChanged, this, [&] {
                if (!session.pairingCode().isEmpty()) serial.write("@REQ 901 PAIR status\n");
            });
        }
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
        QTRY_VERIFY2_WITH_TIMEOUT(selected, qPrintable(session.error() + QString::fromUtf8(QJsonDocument::fromVariant(session.bleDevices()).toJson())),15000);
        QTRY_VERIFY2_WITH_TIMEOUT(session.connected(),qPrintable(serialFailure + session.error() + session.log()),45000);
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
        if (session.info().value("capabilities").toList().contains("companion"))
            QVERIFY2(reply[1].toBool(),qPrintable(reply[3].toString()));
        else {
            QVERIFY(!reply[1].toBool());
            QCOMPARE(reply[3].toString(),QStringLiteral("authenticated_transport_required"));
        }
        QTRY_VERIFY(!session.busy());
        const auto shotId=session.request("SHOT","start"); QVERIFY(shotId);
        QTRY_VERIFY_WITH_TIMEOUT(!replies.isEmpty(),10000);
        reply=replies.takeFirst();QCOMPARE(reply[0].toInt(),shotId);
        QVERIFY(!reply[1].toBool()); QCOMPARE(reply[3].toString(),QString("usb_only"));
        WatchScreen screen(&session); screen.setActive(true);
        QTRY_VERIFY2_WITH_TIMEOUT(screen.available(),qPrintable(screen.state()+session.error()),30000);
        QVERIFY(!screen.capture(QUrl::fromLocalFile("/tmp/ble-capture-must-not-exist.png")));
        screen.setActive(false); QTRY_VERIFY(!session.busy());
        QVERIFY2(serialFailure.isEmpty(),qPrintable(serialFailure));
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
    void deviceViewStateAndActions()
    {
        DeviceSession session;
        CompanionTools tools(&session);
        QCOMPARE(session.viewState(), QStringLiteral("Disconnected"));
        session.startSimulator();
        QTRY_VERIFY(session.connected() && tools.actionsAvailable());
        QCOMPARE(session.viewState(), QStringLiteral("Simulated"));
        tools.setAction("primary", "focus");
        tools.setActionsArmed(true);
        QTest::qWait(1800); // Establish a cursor before generating input.
        tools.emulateButton("primary");
        QTRY_VERIFY_WITH_TIMEOUT(tools.focusRemaining() > 1400, 6000);
        tools.stopFocus();
        QTRY_COMPARE(tools.focusRemaining(), 0);
        tools.setActionsArmed(false);
        tools.emulateButton("primary");
        QTest::qWait(1800);
        QCOMPARE(tools.focusRemaining(), 0);
        session.disconnectDevice();
        QVERIFY(!tools.actionsArmed());
        QCOMPARE(session.viewState(), QStringLiteral("StaleData"));
        tools.setAction("primary", "none");
    }
    void backgroundPollingKeepsUiModelsStable()
    {
        DeviceSession session;
        WatchSettings settings(&session);
        FaceStudio studio(&session);
        CompanionTools tools(&session);
        session.startSimulator();
        QTRY_VERIFY(!settings.faces().isEmpty() && !session.busy());
        QTest::qWait(1200);
        QSignalSpy faces(&settings,&WatchSettings::facesChanged);
        QSignalSpy profiles(&settings,&WatchSettings::profilesChanged);
        QSignalSpy library(&studio,&FaceStudio::libraryChanged);
        QSignalSpy mappings(&tools,&CompanionTools::actionMappingsChanged);
        QSignalSpy timeline(&tools,&CompanionTools::timelineChanged);
        QTest::qWait(2500);
        QCOMPARE(faces.count(),0);
        QCOMPARE(profiles.count(),0);
        QCOMPARE(library.count(),0);
        QCOMPARE(mappings.count(),0);
        QCOMPARE(timeline.count(),0);
        session.disconnectDevice();
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
        studio.editLayer("iconX",20);
        studio.editLayer("iconY",30);
        studio.editLayer("order",5);
        studio.editLayer("animateIcon",true);
        studio.setFrame(1); studio.paint(0,true);
        QCOMPARE(studio.framePixels().left(1),QString("8"));
        studio.editLayer("iconX",280);
        QCOMPARE(studio.layers().value("iconX").toInt(),20);
        QVERIFY(studio.exportDesign(path));
        QVERIFY(studio.importDesign(path));
        QCOMPARE(studio.layers().value("order").toInt(),5);
        QTRY_VERIFY(!session.busy());
        studio.install();
        QTRY_COMPARE(studio.state(),QString("Installed and selected"));
        studio.setFrame(0);
        session.disconnectDevice();
    }
    void settingsDraftSurvivesRefreshAndInFlightApply()
    {
        DeviceSession session; WatchSettings settings(&session); session.startSimulator();
        QTRY_VERIFY(settings.supported() && !settings.timezones().isEmpty() && !settings.faces().isEmpty());
        settings.edit("hour_format",1); settings.refresh();
        QTRY_COMPARE(settings.state(),QString("Pending changes"));
        QCOMPARE(settings.draft().value("hour_format").toInt(),1);
        QTRY_VERIFY(!session.busy());settings.apply();settings.edit("hour_format",0);
        QTRY_COMPARE(settings.values().value("hour_format").toInt(),1);
        QCOMPARE(settings.draft().value("hour_format").toInt(),0); QVERIFY(settings.dirty());
        session.disconnectDevice();session.startSimulator();
        QTRY_VERIFY(settings.supported() && !session.busy());
        QTRY_COMPARE(settings.values().value("hour_format").toInt(),1);
        QCOMPARE(settings.draft().value("hour_format").toInt(),0);
        settings.discard(); QVERIFY(!settings.dirty());session.disconnectDevice();
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
    void visualReview()
    {
        const auto output = qEnvironmentVariable("FLINT_DESIGN_REVIEW_DIR");
        if (output.isEmpty()) QSKIP("Set FLINT_DESIGN_REVIEW_DIR for visual review captures.");
        QDir().mkpath(output);
        Localization localization;
        localization.setLanguage("ru");
        DeviceSession session;
        WatchSettings watchSettings(&session);
        FaceStudio faceStudio(&session);
        CompanionTools companionTools(&session);
        QStringList warnings;
        WatchScreen watchScreen(&session);
        QQmlApplicationEngine engine;
        engine.addImageProvider("watchscreen",new WatchImageProvider(watchScreen.store()));
        engine.addImageProvider("designpreview",new FacePreviewProvider(&faceStudio));
        engine.rootContext()->setContextProperty("designPreviewAvailable",true);
        engine.rootContext()->setContextProperty("watchScreen",&watchScreen);
        connect(&engine,&QQmlEngine::warnings,this,[&](const QList<QQmlError>& errors) { for (const auto& error:errors) warnings.append(error.toString()); });
        engine.rootContext()->setContextProperty("device",&session);
        engine.rootContext()->setContextProperty("watchSettings",&watchSettings);
        engine.rootContext()->setContextProperty("faceStudio",&faceStudio);
        engine.rootContext()->setContextProperty("companionTools",&companionTools);
        engine.rootContext()->setContextProperty("localization",&localization);
        const auto port = qEnvironmentVariable("FLINT_REVIEW_USB_PORT");
        if (port.isEmpty()) session.startSimulator(); else session.connectPort(port);
        QTRY_VERIFY2_WITH_TIMEOUT(session.connected() && !watchSettings.faces().isEmpty(),qPrintable(session.error() + session.log()),10000);
        engine.load(QUrl("qrc:/qt/qml/Flint/Desktop/Main.qml"));
        QVERIFY2(!engine.rootObjects().isEmpty(),qPrintable(warnings.join('\n')));
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        QVERIFY(window);
        window->resize(1440,900);
        auto* theme = window->findChild<QObject*>("themeSelector");
        QVERIFY(theme);
        theme->setProperty("currentIndex",2);
        QVERIFY(QMetaObject::invokeMethod(theme,"activated",Q_ARG(int,2)));
        for (int section=0;section<7;++section) {
            window->setProperty("section",section); QTest::qWait(180);
            if (section == 1) {
                auto* hourChoice = window->findChild<QObject*>("choice-hour_format");
                QVERIFY(hourChoice);
                QCOMPARE(hourChoice->property("currentIndex").toInt(),watchSettings.draft().value("hour_format").toInt());
                QVERIFY(!hourChoice->property("currentText").toString().isEmpty());
            }
            QVERIFY(window->grabWindow().save(output+QString("/page-%1.png").arg(section)));
        }
        window->requestActivate();
        window->setProperty("section",0);
        QTest::qWait(50);
        QTest::keyClick(window,Qt::Key_3,Qt::AltModifier);
        QTRY_COMPARE(window->property("section").toInt(),2);
        QTest::mouseClick(window,Qt::BackButton,Qt::NoModifier,QPoint(900,70));
        QTRY_COMPARE(window->property("section").toInt(),0);
        QTest::mouseClick(window,Qt::ForwardButton,Qt::NoModifier,QPoint(900,70));
        QTRY_COMPARE(window->property("section").toInt(),2);
        window->setProperty("section",0);
        QTest::qWait(50);
        auto* touch = QTest::createTouchDevice();
        QTest::touchEvent(window,touch).press(0,QPoint(1250,780));
        QTest::qWait(30);
        QTest::touchEvent(window,touch).move(0,QPoint(1000,780));
        QTest::qWait(30);
        QTest::touchEvent(window,touch).move(0,QPoint(550,780));
        QTest::qWait(30);
        QTest::touchEvent(window,touch).release(0,QPoint(550,780));
        QTRY_COMPARE(window->property("section").toInt(),1);
        QTest::qWait(500);
        const auto wheelAt=[&](QPointF point) {
            QWheelEvent event(point,window->mapToGlobal(point.toPoint()),QPoint(-100,0),QPoint(-120,0),Qt::NoButton,Qt::NoModifier,Qt::ScrollUpdate,false);
            QCoreApplication::sendEvent(window,&event);
        };
        wheelAt(QPointF(1220,400));
        QTest::qWait(50); QCOMPARE(window->property("section").toInt(),1);
        wheelAt(QPointF(700,50));
        QTRY_COMPARE(window->property("section").toInt(),2);
        QVERIFY2(warnings.isEmpty(),qPrintable(warnings.join('\n')));
        session.disconnectDevice();
    }
    void responsiveAndLocalized()
    {
        Localization localization;
        DeviceSession session;
        WatchSettings watchSettings(&session);
        FaceStudio faceStudio(&session);
        CompanionTools companionTools(&session);
        QStringList warnings;
        WatchScreen watchScreen(&session);
        QQmlApplicationEngine engine;
        engine.addImageProvider("watchscreen",new WatchImageProvider(watchScreen.store()));
        engine.addImageProvider("designpreview",new FacePreviewProvider(&faceStudio));
        engine.rootContext()->setContextProperty("designPreviewAvailable",true);
        engine.rootContext()->setContextProperty("watchScreen",&watchScreen);
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
                signal requestFinished(int id, bool okay, var result, string error)
                property string viewState: "Connected"
                property string pairingCode: ""
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
                function request(operation, arguments) { return 0 }
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
        auto restoreSession = qScopeGuard([&] { window->setProperty("session",QVariant::fromValue(&session)); });
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
                    for (int page = 0; page < 7; ++page) {
                        window->setProperty("section", page);
                        QTest::qWait(80);
                        QCOMPARE(window->property("compact").toBool(), size.width() < 900);
                        QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join('\n')));
                        const auto overflow = [&](auto&& self, QQuickItem* item) -> QString {
                            if (!item->isVisible() || (item->property("inSwipeView").toBool() && !item->property("currentSwipePage").toBool()))
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
