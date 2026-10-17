#include "device/DeviceSession.h"
#include "localization/Localization.h"

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
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
        QStringList warnings;
        QQmlApplicationEngine engine;
        connect(&engine, &QQmlEngine::warnings, this, [&](const QList<QQmlError>& errors) {
            for (const auto& error : errors)
                warnings.append(error.toString());
        });
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
                property bool connected: true
                property bool busy: false
                property bool polling: true
                property string connectionState: "Connected"
                property string error: ""
                property string notification: ""
                property int sampleCount: 42
                property string lastUpdated: "2026-10-17T19:30:00Z"
                property string log: "USB connected\n> @STATUS\nFLINT STATUS {...}"
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
        for (const auto& language : { QStringLiteral("ru"), QStringLiteral("en") }) {
            localization.setLanguage(language);
            QTest::qWait(1);
            QCOMPARE(selector->property("currentValue").toString(), language);
            QCOMPARE(selector->property("currentText").toString(), language == "ru" ? "Русский" : "English");
            QCOMPARE(QCoreApplication::translate("AppNavigation", "Overview"), language == "ru" ? "Обзор" : "Overview");
            QCOMPARE(QCoreApplication::translate("DeviceSession", "Connected"), language == "ru" ? "Подключено" : "Connected");
            for (const QSize size : { QSize(540, 640), QSize(760, 700), QSize(1024, 768), QSize(1440, 900) }) {
                window->resize(size);
                // Test both the empty state and populated telemetry at every breakpoint.
                for (bool hasData : { false, true }) {
                    window->setProperty("session", QVariant::fromValue(hasData ? populated.data() : &session));
                    for (int page = 0; page < 3; ++page) {
                        window->setProperty("section", page);
                        QTest::qWait(80);
                        QCOMPARE(window->property("compact").toBool(), size.width() < 900);
                        QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join('\n')));
                        const auto check = [&](auto&& self, QQuickItem* item) -> void {
                            if (!item->isVisible())
                                return;
                            // Ignore vertically scrollable content; horizontal overflow is never intended.
                            if (item->metaObject()->indexOfProperty("text") >= 0 && item->width() > 0) {
                                const auto position = item->mapToItem(window->contentItem(), QPointF());
                                QVERIFY2(position.x() >= -1 && position.x() + item->width() <= window->width() + 1,
                                    qPrintable(QString("Overflow: %1 at %2, width %3, viewport %4")
                                            .arg(item->property("text").toString())
                                            .arg(position.x())
                                            .arg(item->width())
                                            .arg(window->width())));
                            }
                            for (auto* child : item->childItems())
                                self(self, child);
                        };
                        check(check, window->contentItem());
                        const auto output = qEnvironmentVariable("FLINT_SCREENSHOT_DIR");
                        if (!output.isEmpty())
                            window->grabWindow().save(QString("%1/%2-%3-%4-%5.png").arg(output, language).arg(size.width()).arg(page).arg(hasData ? "connected" : "empty"));
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
