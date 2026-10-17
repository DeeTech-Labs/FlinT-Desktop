#include "Localization.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonDocument>
#include <QLocale>
#include <QMap>
#include <QSettings>
#include <QStandardPaths>

namespace {
QJsonObject readCatalog(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}
}

QString Localization::userDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/translations");
}

Localization::Localization(QObject* parent)
    : QTranslator(parent)
{
    QMap<QString, QString> catalog;
    for (const QString& directory : { QStringLiteral(":/translations"), userDirectory() }) {
        const QDir dir(directory);
        for (const QString& code : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            if (code.contains(".."))
                continue;
            const auto root = readCatalog(dir.filePath(code + QStringLiteral("/meta.json")));
            if (root.value("language").toString() != code)
                continue;
            catalog[code] = root.value("name").toString(code);
        }
    }
    for (auto it = catalog.cbegin(); it != catalog.cend(); ++it)
        m_languages.append(QVariantMap { { "code", it.key() }, { "name", it.value() } });
    const QString system = QLocale::system().name().section('_', 0, 0);
    setLanguage(QSettings().value(QStringLiteral("interface/language"),
                               catalog.contains(system) ? system : QStringLiteral("en"))
            .toString());
    QCoreApplication::installTranslator(this);
}

void Localization::merge(const QString& path)
{
    const auto contexts = readCatalog(path).value("contexts").toObject();
    for (auto context = contexts.begin(); context != contexts.end(); ++context) {
        auto messages = m_contexts.value(context.key()).toObject();
        const auto incoming = context.value().toObject();
        for (auto entry = incoming.begin(); entry != incoming.end(); ++entry)
            if (entry.value().isString() && !entry.value().toString().isEmpty())
                messages.insert(entry.key(), entry.value());
        m_contexts.insert(context.key(), messages);
    }
}

void Localization::mergeLanguage(const QString& directory, const QString& language)
{
    const QString path = directory + '/' + language;
    QDirIterator files(path, { QStringLiteral("*.json") }, QDir::Files, QDirIterator::Subdirectories);
    QStringList paths;
    while (files.hasNext()) {
        const auto filename = files.next();
        if (files.fileName() != QStringLiteral("meta.json")
            && !files.fileName().endsWith(QStringLiteral(".example.json")))
            paths.append(filename);
    }
    // Deterministic ordering on every platform; duplicate keys are rejected by tests.
    paths.sort();
    for (const auto& file : paths)
        merge(file);
}

void Localization::setLanguage(const QString& language)
{
    bool known = false;
    for (const auto& item : m_languages)
        known = known || item.toMap().value("code").toString() == language;
    const auto selected = known ? language : QStringLiteral("en");
    if (selected == m_language)
        return;
    m_contexts = {};
    // Missing custom translations fall back to bundled English, then the source.
    mergeLanguage(QStringLiteral(":/translations"), QStringLiteral("en"));
    mergeLanguage(QStringLiteral(":/translations"), selected);
    mergeLanguage(userDirectory(), selected);
    m_language = selected;
    QSettings().setValue(QStringLiteral("interface/language"), selected);
    emit languageChanged();
}

QString Localization::translate(const char* context, const char* sourceText,
    const char* disambiguation, int n) const
{
    Q_UNUSED(disambiguation)
    Q_UNUSED(n)
    if (!context || !sourceText)
        return {};
    return m_contexts.value(QString::fromUtf8(context)).toObject().value(QString::fromUtf8(sourceText)).toString();
}
