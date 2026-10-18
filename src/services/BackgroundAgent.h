#pragma once
#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
class BackgroundAgent : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY changed)
public:
    explicit BackgroundAgent(QObject* parent = nullptr);
    bool available() const { return QSystemTrayIcon::isSystemTrayAvailable(); }
    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);
signals:
    void changed();
    void showWindow();
private:
    QMenu m_menu;
    QSystemTrayIcon m_tray;
    bool m_enabled = false;
};
