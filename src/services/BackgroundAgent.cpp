#include "BackgroundAgent.h"
#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QSettings>
BackgroundAgent::BackgroundAgent(QObject* parent) : QObject(parent) {
    QPixmap icon(32,32); icon.fill(Qt::transparent);
    QPainter painter(&icon); painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath spark; spark.moveTo(18,2); spark.lineTo(6,18); spark.lineTo(14,18); spark.lineTo(10,30); spark.lineTo(27,12); spark.lineTo(18,12); spark.closeSubpath();
    painter.fillPath(spark,QColor("#d4fa70")); painter.end();
    m_tray.setIcon(QIcon(icon)); m_tray.setToolTip("FlinT Desktop");
    m_menu.addAction(tr("Open FlinT Desktop"),this,&BackgroundAgent::showWindow);
    m_menu.addSeparator();
    m_menu.addAction(tr("Quit"),qApp,&QApplication::quit);
    m_tray.setContextMenu(&m_menu);
    connect(&m_tray,&QSystemTrayIcon::activated,this,[this](QSystemTrayIcon::ActivationReason reason) { if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) emit showWindow(); });
    setEnabled(QSettings().value("companion/background",false).toBool());
}
void BackgroundAgent::setEnabled(bool enabled) {
    m_enabled = enabled && available();
    m_tray.setVisible(m_enabled);
    qApp->setQuitOnLastWindowClosed(!m_enabled);
    QSettings().setValue("companion/background",m_enabled);
    if (!m_enabled) emit showWindow();
    emit changed();
}
