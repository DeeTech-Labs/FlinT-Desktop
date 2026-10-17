#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCommandLineParser>
#include <QJsonDocument>
#include <QTimer>
#include <cstdio>
#include "device/DeviceSession.h"

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("DeeTech Labs"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("deetech.labs"));
    QCoreApplication::setApplicationName(QStringLiteral("FlinT Desktop"));
    QCoreApplication::setApplicationVersion(QStringLiteral(FLINT_DESKTOP_VERSION));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("FlinT OS USB device companion"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption probeOption(QStringLiteral("probe"),
        QStringLiteral("Collect device information and status as JSON, then exit."), QStringLiteral("port"));
    const QCommandLineOption syncOption(QStringLiteral("sync-time"),
        QStringLiteral("Synchronize time before returning --probe data."));
    parser.addOption(probeOption);
    parser.addOption(syncOption);
    const QCommandLineOption outputOption(QStringLiteral("output"),
        QStringLiteral("Save --probe capture to a JSON file."), QStringLiteral("file"));
    parser.addOption(outputOption);
    const QCommandLineOption samplesOption(QStringLiteral("samples"),
        QStringLiteral("Minimum number of status snapshots for --probe (1..600)."),
        QStringLiteral("count"), QStringLiteral("1"));
    parser.addOption(samplesOption);
    parser.process(app);
    if (!parser.isSet(probeOption) && (parser.isSet(syncOption) || parser.isSet(outputOption)
                                     || parser.isSet(samplesOption)))
        parser.showHelp(2);
    bool countValid = false;
    const int sampleTarget = parser.value(samplesOption).toInt(&countValid);
    if (!countValid || sampleTarget < 1 || sampleTarget > 600) parser.showHelp(2);
    DeviceSession device;
    if (parser.isSet(probeOption)) {
        bool requestedSync = false;
        QObject::connect(&device, &DeviceSession::failed, &app, [&app](const QString& message) {
            std::fprintf(stderr, "%s\n", qPrintable(message));
            app.exit(2);
        });
        QObject::connect(&device, &DeviceSession::sampleReceived, &app, [&] {
            if (parser.isSet(syncOption) && !requestedSync) {
                requestedSync = true;
                if (!device.info().value("capabilities").toList().contains("time_sync")) {
                    std::fprintf(stderr, "Device does not support time sync\n");
                    app.exit(2);
                    return;
                }
                device.syncTime();
                return;
            }
            if (device.sampleCount() < sampleTarget) return;
            if (parser.isSet(outputOption)
                && !device.saveData(QUrl::fromLocalFile(parser.value(outputOption)))) {
                std::fprintf(stderr, "%s\n", qPrintable(device.error()));
                app.exit(2);
                return;
            }
            const auto output = QJsonDocument(device.capture()).toJson(QJsonDocument::Indented);
            std::fwrite(output.constData(), 1, output.size(), stdout);
            app.exit(0);
        });
        QTimer::singleShot(0, &device, [&] { device.connectPort(parser.value(probeOption)); });
        QTimer::singleShot(15000 + sampleTarget * 3000, &app, [&app] {
            std::fprintf(stderr, "Device probe timed out\n");
            app.exit(2);
        });
        return app.exec();
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("device"), &device);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("Flint.Desktop", "Main");

    return app.exec();
}
