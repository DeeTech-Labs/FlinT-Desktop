#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("DeeTech Labs"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("deetech.labs"));
    QCoreApplication::setApplicationName(QStringLiteral("FlinT Desktop"));
    QCoreApplication::setApplicationVersion(QStringLiteral(FLINT_DESKTOP_VERSION));

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("Flint.Desktop", "Main");

    return app.exec();
}
