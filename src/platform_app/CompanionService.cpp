#include "CompanionService.h"
#include <QFile>
#include <QRegularExpression>
CompanionService::CompanionService(PlatformClient *client) : client_(client) {
    timer_.setInterval(2000);
    connect(&timer_, &QTimer::timeout, this, &CompanionService::sample);
    connect(client, &PlatformClient::ready, this, [this] { timer_.start(); });
    connect(client, &PlatformClient::disconnected, this, [this] {
        timer_.stop();
        allowed_ = false;
        status_ = "Передача CPU/RAM выключена";
        emit changed();
    });
}
void CompanionService::setMetricsAllowed(bool value) {
    allowed_ = value;
    status_ = value ? "Разрешено локально; требуется право выбранного Ignite"
                    : "Передача CPU/RAM выключена";
    emit changed();
    if (value)
        sample();
}
void CompanionService::sample() {
    if (!allowed_ || !client_->connected())
        return;
    auto active = client_->info().value("active").toString();
    if (active.isEmpty())
        return;
    flint::Writer w;
    w.text(active.toStdString());
    client_->request(
        flint::Method::permissions_get, w.data, [this, active](auto error, auto bytes) {
            if (!allowed_ || error != flint::Error::ok ||
                active != client_->info().value("active").toString())
                return;
            flint::Reader r{bytes};
            auto count = r.n(2);
            if (count > 64)
                return;
            bool permitted = false;
            for (unsigned i = 0; i < count; ++i) {
                auto name = r.text(80), scope = r.text(192);
                r.n(1);
                bool enabled = r.n(1);
                if (name == "companion.pc.metrics" && scope == "cpu,ram" && enabled)
                    permitted = true;
            }
            if (!r.done() || !permitted) {
                status_ = "Ignite не получил право CPU/RAM";
                emit changed();
                return;
            }
#ifdef Q_OS_LINUX
            QFile stat("/proc/stat"), memory("/proc/meminfo");
            if (!stat.open(QIODevice::ReadOnly) || !memory.open(QIODevice::ReadOnly))
                return;
            auto fields = QString::fromUtf8(stat.readLine()).simplified().split(' ');
            if (fields.size() < 5)
                return;
            quint64 total = 0;
            for (int i = 1; i < fields.size() && i <= 8; ++i)
                total += fields[i].toULongLong();
            auto idle = fields[4].toULongLong() + (fields.size() > 5 ? fields[5].toULongLong() : 0);
            double cpu =
                total > total_ ? 100.0 * (1.0 - double(idle - idle_) / double(total - total_)) : 0;
            total_ = total;
            idle_ = idle;
            auto text = QString::fromUtf8(memory.readAll());
            auto all = QRegularExpression("MemTotal:\\s+(\\d+)").match(text),
                 free = QRegularExpression("MemAvailable:\\s+(\\d+)").match(text);
            if (!all.hasMatch() || !free.hasMatch() || !all.captured(1).toDouble())
                return;
            auto ram = 100 * (1 - free.captured(1).toDouble() / all.captured(1).toDouble());
            flint::Writer metrics;
            metrics.n(qBound(0, int(cpu * 100), 10000));
            metrics.n(qBound(0, int(ram * 100), 10000));
            if (!allowed_)
                return;
            client_->request(flint::Method::metrics, metrics.data, [this](auto e, auto) {
                status_ = e == flint::Error::ok ? "CPU/RAM передаются активному Ignite"
                                                : platformError(e);
                emit changed();
            });
#else
 status_="Сбор CPU/RAM доступен на Linux";emit changed();
#endif
        });
}
