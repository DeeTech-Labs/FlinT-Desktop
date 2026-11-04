#include "Preview.h"
#include <QFile>
Preview::Preview(PlatformClient *client)
    : client_(client), cache_(std::make_unique<QTemporaryDir>()) {
    image_ = QImage(280, 240, QImage::Format_RGB32);
    image_.fill(QColor("#080d18"));
    timer_.setInterval(1000);
    connect(&timer_, &QTimer::timeout, this, &Preview::refresh);
    connect(client, &PlatformClient::ready, this, [this] {
        if (pollingActive_) {
            timer_.start();
            refresh();
        }
    });
    connect(client, &PlatformClient::disconnected, this, [this] {
        timer_.stop();
        busy_ = false;
        active_.clear();
        runtime_.stop();
        image_.fill(QColor("#080d18"));
        ++revision_;
        emit changed();
    });
}
void Preview::refresh() {
    if (!client_->connected() || busy_)
        return;
    busy_ = true;
    if (client_->simulated()) {
        scene();
        return;
    }
    auto active = client_->info().value("active").toString();
    auto component = client_->info().value("component").toString();
    auto key = active + "/" + component + "/" + client_->info().value("generation").toString();
    if (key == active_) {
        scene();
        return;
    }
    active_ = key;
    download_.clear();
    total_ = 0;
    runtime_.stop();
    download();
}
void Preview::download() {
    flint::Writer w;
    w.text(active_.section('/', 0, 0).toStdString());
    w.n(download_.size());
    client_->request(flint::Method::package_read, w.data, [this](auto error, auto bytes) {
        if (error != flint::Error::ok) {
            busy_ = false;
            status_ = platformError(error);
            emit changed();
            return;
        }
        flint::Reader r{bytes};
        auto total = r.n();
        if (!r.valid || total > flint::MaxPackageBytes || !total || (total_ && total_ != total) ||
            bytes.size() <= 4) {
            busy_ = false;
            active_.clear();
            return;
        }
        total_ = total;
        download_.insert(download_.end(), bytes.begin() + 4, bytes.end());
        if (download_.size() > total_) {
            busy_ = false;
            active_.clear();
            return;
        }
        if (download_.size() < total_) {
            download();
            return;
        }
        auto path = cache_->path() + "/preview.ignit";
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly) ||
            file.write(reinterpret_cast<const char *>(download_.data()), download_.size()) !=
                qint64(download_.size())) {
            busy_ = false;
            return;
        }
        file.close();
        flint::Installed p;
        p.path = path.toStdString();
        auto error2 = runtime_.launch(
            p, active_.section('/', 1, 1).toStdString(), {48 * 1024, 4 * 1024 * 1024, 4096}, 63);
        download_.clear();
        if (error2 != flint::Error::ok) {
            busy_ = false;
            active_.clear();
            status_ = platformError(error2);
            emit changed();
            return;
        }
        scene();
    });
}
void Preview::scene() {
    client_->request(flint::Method::scene, {}, [this](auto error, auto bytes) {
        busy_ = false;
        if (error != flint::Error::ok) {
            status_ = platformError(error);
            emit changed();
            return;
        }
        FlintRender::Scene s;
        if (!FlintRender::decode(bytes.data(), bytes.size(), s)) {
            status_ = "Некорректная сцена";
            emit changed();
            return;
        }
        paint(s);
        status_ = "Общий Ignite Renderer";
        ++revision_;
        emit changed();
    });
}
void Preview::paint(const FlintRender::Scene &scene) {
    std::vector<uint16_t> pixels(280 * 240);
    FlintRender::Canvas canvas(pixels.data(), 280, 240);
    if (auto *core = client_->simulator())
        core->paint(scene, canvas);
    else {
        FlintRender::resourceContext = &runtime_.resources();
        FlintRender::resourceGlyph = [](void *c, unsigned f, uint32_t cp) {
            return static_cast<flint::ResourceManager *>(c)->glyph(f, cp);
        };
        FlintRender::resourceImage = [](void *c, FlintRender::Canvas &canvas,
                                         const FlintRender::Op &op) {
            static_cast<flint::ResourceManager *>(c)->image(canvas, op);
        };
        FlintRender::render(scene, canvas);
        FlintRender::resourceContext = nullptr;
        FlintRender::resourceGlyph = nullptr;
        FlintRender::resourceImage = nullptr;
    }
    for (int y = 0; y < 240; ++y)
        for (int x = 0; x < 280; ++x) {
            auto c = pixels[y * 280 + x];
            image_.setPixel(x, y,
                qRgb(((c >> 11) & 31) * 255 / 31, ((c >> 5) & 63) * 255 / 63, (c & 31) * 255 / 31));
        }
}

bool Preview::capture(const QUrl &url) {
    return url.isLocalFile() && image_.save(url.toLocalFile());
}

void Preview::setActive(bool active) {
    if (pollingActive_ == active) {
        return;
    }
    pollingActive_ = active;
    if (active && client_->connected()) {
        timer_.start();
        refresh();
    } else {
        timer_.stop();
    }
    emit changed();
}
