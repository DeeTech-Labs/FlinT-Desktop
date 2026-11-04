#pragma once
#include <QObject>
#include <QProcess>
#include <QTemporaryDir>
#include <QUrl>
class Studio : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QUrl output READ output NOTIFY changed)
public:
    explicit Studio(QObject *parent = nullptr);
    bool busy() const {
        return process_.state() != QProcess::NotRunning;
    }
    QString message() const {
        return message_;
    }
    QUrl output() const {
        return output_;
    }
    Q_INVOKABLE void build(const QUrl &project, const QUrl &output);
signals:
    void changed();

private:
    QProcess process_;
    QTemporaryDir tools_;
    QString message_;
    QUrl output_;
};
