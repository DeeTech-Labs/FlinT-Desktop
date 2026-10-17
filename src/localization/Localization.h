#pragma once

#include <QJsonObject>
#include <QTranslator>
#include <QVariantList>

// PixelStudio / Rebit-compatible JSON catalogs; no generated .qm files required.
class Localization : public QTranslator {
    Q_OBJECT
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QVariantList languages READ languages CONSTANT)
public:
    explicit Localization(QObject* parent = nullptr);
    QString language() const { return m_language; }
    QVariantList languages() const { return m_languages; }
    void setLanguage(const QString& language);
    bool isEmpty() const override { return false; }
    QString translate(const char* context, const char* sourceText,
        const char* disambiguation = nullptr, int n = -1) const override;
    static QString userDirectory();
signals:
    void languageChanged();

private:
    void merge(const QString& path);
    void mergeLanguage(const QString& directory, const QString& language);
    QString m_language;
    QVariantList m_languages;
    QJsonObject m_contexts;
};
