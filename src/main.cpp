#include "localization/Localization.h"
#include "platform_app/CompanionService.h"
#include "platform_app/IgniteHub.h"
#include "platform_app/PlatformClient.h"
#include "platform_app/Preview.h"
#include "platform_app/Studio.h"
#include "services/BackgroundAgent.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrl>
#include <cstdio>
int main(int argc, char **argv) {
    QQuickStyle::setStyle("Basic");
    QApplication app(argc, argv);
    app.setOrganizationName("DeeTech Labs");
    app.setApplicationName("Flint Flare");
    app.setApplicationVersion(FLINT_FLARE_VERSION);
    QCommandLineParser args;
    args.addHelpOption();
    args.addVersionOption();
    args.addOption({"smoke", "Run the offline simulator/QML verification and exit."});
    args.addOption({"capture", "Save an offline UI screenshot.", "path"});
    args.process(app);
    std::unique_ptr<QTemporaryDir> testPreferences;
    if (args.isSet("smoke")) {
        testPreferences = std::make_unique<QTemporaryDir>();
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, testPreferences->path());
    }
    if (!args.isSet("smoke")) {
        QSettings previous("DeeTech Labs", "FlinT Desktop");
        QSettings current;
        if (!current.value("migration/desktopSettings", false).toBool()) {
            for (const auto &key : previous.allKeys()) {
                if (!current.contains(key))
                    current.setValue(key, previous.value(key));
            }
            current.setValue("migration/desktopSettings", true);
            current.sync();
        }
    }
    Localization localization;
    BackgroundAgent backgroundAgent;
    PlatformClient client;
    IgniteHub hub(&client);
    Preview preview(&client);
    CompanionService companion(&client);
    Studio studio;
    QQmlApplicationEngine engine;
    engine.addImageProvider("ignite", new PreviewProvider(&preview));
    engine.rootContext()->setContextProperty("client", &client);
    engine.rootContext()->setContextProperty("localization", &localization);
    engine.rootContext()->setContextProperty("backgroundAgent", &backgroundAgent);
    QObject::connect(
        &localization, &Localization::languageChanged, &engine, &QQmlEngine::retranslate);
    engine.rootContext()->setContextProperty("hub", &hub);
    engine.rootContext()->setContextProperty("preview", &preview);
    engine.rootContext()->setContextProperty("companion", &companion);
    engine.rootContext()->setContextProperty("studio", &studio);
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("Flint.Flare", "Main");
    QObject::connect(&backgroundAgent, &BackgroundAgent::showWindow, &engine, [&engine] {
        if (auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0))) {
            window->show();
            window->raise();
            window->requestActivate();
        }
    });
    if (args.isSet("smoke")) {
        QObject::connect(&client, &PlatformClient::failure, &app, [&](const QString &message) {
            fprintf(stderr, "%s\n", qPrintable(message));
            app.exit(2);
        });
        struct OfflineState {
            QTemporaryDir files;
            int phase = 0;
            int page = 0;
        };
        const auto state = std::make_shared<OfflineState>();
        auto *check = new QTimer(&app);
        check->setInterval(100);
        QObject::connect(check, &QTimer::timeout, &app, [&, state, check] {
            if (!client.connected())
                return;
            const QString sampleId = "example.pc-monitor";
            bool installed = false;
            for (const auto &entry : hub.packages()) {
                if (entry.toMap().value("id").toString() == sampleId)
                    installed = true;
            }
            bool consentKnown = false;
            bool consentEnabled = false;
            for (const auto &entry : hub.permissions()) {
                const auto permission = entry.toMap();
                if (permission.value("name") == "companion.pc.metrics") {
                    consentKnown = true;
                    consentEnabled = permission.value("enabled").toBool();
                }
            }
            switch (state->phase) {
            case 0: {
                if (hub.packages().size() != 6 || preview.status().isEmpty() ||
                    client.info().value("epoch").toULongLong() < 1700000000ULL)
                    return;
                const auto path = state->files.path() + "/sample.ignit";
                if (!QFile::copy(":/samples/sample.ignit", path)) {
                    app.exit(5);
                    return;
                }
                state->phase = 1;
                hub.install(QUrl::fromLocalFile(path));
                return;
            }
            case 1:
                if (!installed || hub.busy())
                    return;
                state->phase = 2;
                hub.launch(sampleId, "main");
                return;
            case 2:
                if (client.info().value("active") != sampleId)
                    return;
                state->phase = 3;
                hub.select(sampleId);
                return;
            case 3:
                if (!consentKnown)
                    return;
                state->phase = 4;
                hub.grant("companion.pc.metrics", "cpu,ram", true);
                return;
            case 4:
                if (!consentEnabled)
                    return;
                state->phase = -1;
                {
                    flint::Writer sample;
                    sample.n(2500);
                    sample.n(5000);
                    client.request(
                        flint::Method::metrics, sample.data, [&, state](auto error, auto) {
                            if (error != flint::Error::ok) {
                                app.exit(6);
                                return;
                            }
                            state->phase = 5;
                        });
                }
                return;
            case 5:
                state->phase = 6;
                hub.grant("companion.pc.metrics", "cpu,ram", false);
                return;
            case 6:
                if (consentEnabled)
                    return;
                state->phase = 7;
                hub.remove(sampleId);
                return;
            case 7:
                if (installed || hub.packages().size() != 6 || hub.busy())
                    return;
                state->phase = 8;
                return;
            case 8: {
                auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
                auto *pages = window ? window->findChild<QObject *>("pageStack") : nullptr;
                if (!window || !pages || pages->property("count").toInt() != 7) {
                    app.exit(7);
                    return;
                }
                window->setWidth(state->page < 7 ? 1320 : 540);
                window->setProperty("section", state->page % 7);
                if (++state->page == 14) {
                    state->phase = 9;
                    window->setWidth(1320);
                    window->setProperty("section", 0);
                }
                return;
            }
            case 9:
                check->stop();
                QTimer::singleShot(300, &app, [&] {
                    if (args.isSet("capture")) {
                        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
                        if (!window || !window->grabWindow().save(args.value("capture"))) {
                            app.exit(3);
                            return;
                        }
                    }
                    printf(
                        "Offline QML, all seven pages at two widths, binary handshake, Hub "
                        "install/launch/remove, consent/revocation and shared preview: passed\n");
                    app.exit(0);
                });
                return;
            default:
                return;
            }
        });
        check->start();
        QTimer::singleShot(0, &client, &PlatformClient::startSimulator);
        QTimer::singleShot(15000, &app, [&] {
            fprintf(stderr, "Offline smoke test timed out: %s\n", qPrintable(client.status()));
            app.exit(4);
        });
    }
    return app.exec();
}
