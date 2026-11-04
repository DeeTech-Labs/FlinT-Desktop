#include "PlatformClient.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSerialPortInfo>
#include <QSettings>
#include <QStandardPaths>
#include <chrono>
static uint64_t monotonic() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
QString platformError(flint::Error e) {
    static const char *names[] = {"Готово", "Некорректный запрос", "Повреждённый пакет",
        "Несовместимая сборка", "Не поддерживается", "Доступ запрещён",
        "Требуется согласие пользователя", "Источник недоступен", "Отменено", "Превышена квота",
        "Недостаточно памяти", "Ошибка хранения", "Нарушена целостность", "Некорректное состояние",
        "Устаревший ресурс", "Превышен лимит выполнения"};
    auto i = unsigned(e);
    return QString::fromUtf8(i < std::size(names) ? names[i] : "Ошибка протокола");
}
PlatformClient::PlatformClient(QObject *parent) : QObject(parent) {
    simulationTick_.setInterval(33);
    connect(&simulationTick_, &QTimer::timeout, this, [this] {
        if (core_) core_->tick();
    });
    simulationTick_.start();
    pairingPoll_.setInterval(1000);
    connect(&pairingPoll_, &QTimer::timeout, this, [this] {
        if (serial_.isOpen())
            pairing("status");
    });
    timeout_.setSingleShot(true);
    timeout_.setInterval(45000);
    connect(&timeout_, &QTimer::timeout, this, [this] { fail("Устройство не ответило вовремя"); });
    connect(&serial_, &QSerialPort::readyRead, this, [this] { receive(serial_.readAll()); });
    connect(&serial_, &QSerialPort::errorOccurred, this, [this](auto error) {
        if (error == QSerialPort::ResourceError)
            fail(serial_.errorString());
    });
}
QVariantList PlatformClient::ports() const {
    QVariantList result;
    for (auto &p : QSerialPortInfo::availablePorts())
        result.append(QVariantMap{
            {"path", p.systemLocation()}, {"label", p.portName() + " · " + p.description()}});
    return result;
}
QVariantList PlatformClient::bleDevices() const {
    return ble_ ? ble_->devices() : QVariantList{};
}
QString PlatformClient::pairingCode() const {
    return ble_ ? ble_->pairingCode() : QString{};
}
void PlatformClient::ensureBle() {
    if (ble_)
        return;
    ble_ = std::make_unique<BleTransport>();
    connect(ble_.get(), &BleTransport::devicesChanged, this, &PlatformClient::changed);
    connect(ble_.get(), &BleTransport::pairingCodeChanged, this, &PlatformClient::changed);
    connect(ble_.get(), &BleTransport::received, this, &PlatformClient::receive);
    connect(ble_.get(), &BleTransport::error, this, &PlatformClient::fail);
    connect(ble_.get(), &BleTransport::readyChanged, this, [this] {
        if (ble_->ready())
            handshake();
    });
}
void PlatformClient::disconnectDevice() {
    ++generation_;
    pairingPoll_.stop();
    watchPairingCode_.clear();
    timeout_.stop();
    connected_ = false;
    capabilities_ = 0;
    if (core_)
        core_->disconnect(session_);
    queue_.clear();
    pending_.reset();
    serial_.close();
    if (ble_)
        ble_->close();
    core_.reset();
    simulation_.reset();
    parser_.reset();
    info_.clear();
    status_ = "Отключено";
    emit disconnected();
    emit changed();
}
void PlatformClient::fail(const QString &message) {
    disconnectDevice();
    status_ = message;
    emit changed();
    emit failure(message);
}
void PlatformClient::connectUsb(const QString &port) {
    disconnectDevice();
    serial_.setPortName(port);
    serial_.setBaudRate(115200);
    if (!serial_.open(QIODevice::ReadWrite)) {
        fail(serial_.errorString());
        return;
    }
    handshake();
}
void PlatformClient::scanBle() {
    ensureBle();
    ble_->scan();
    emit changed();
}
void PlatformClient::connectBle(const QString &address) {
    if (serial_.isOpen()) {
        // Retain the authenticated local USB channel for numeric comparison.
        timeout_.stop();
        connected_ = false;
        queue_.clear();
        pending_.reset();
        parser_.reset();
        info_.clear();
        emit disconnected();
    } else {
        disconnectDevice();
    }
    ensureBle();
    ble_->open(address);
    status_ = "Подключение BLE…";
    emit changed();
}
void PlatformClient::confirmPairing(bool accepted) {
    if (ble_)
        ble_->confirmPairing(accepted);
}
void PlatformClient::pairing(const QString &action) {
    if (!serial_.isOpen()) {
        emit failure(tr("Pairing requires a local USB connection."));
        return;
    }
    flint::Writer writer;
    writer.text(action.toStdString());
    request(flint::Method::pairing, writer.data, [this, action](auto error, auto data) {
        if (error != flint::Error::ok) {
            pairingPoll_.stop();
            emit failure(platformError(error));
            return;
        }
        const auto state =
            QString::fromUtf8(reinterpret_cast<const char *>(data.data()), data.size());
        const auto code = QRegularExpression("code=(\\d+)").match(state);
        watchPairingCode_ = code.hasMatch() ? code.captured(1).rightJustified(6, '0') : QString();
        if (state.startsWith("open=1")) {
            pairingPoll_.start();
        } else {
            pairingPoll_.stop();
        }
        if (action != "status") {
            status_ = state;
        }
        emit changed();
    });
}
void PlatformClient::startSimulator() {
    disconnectDevice();
    simulation_ = std::make_unique<QTemporaryDir>();
    auto root = simulation_->path();
    QDir().mkpath(root + "/packages");
    QDir().mkpath(root + "/factory");
    // Factory binaries are built by the same compiler as the firmware image.
    for (auto &name : QDir(":/factory").entryList({"*.ignit"}, QDir::Files))
        QFile::copy(":/factory/" + name, root + "/factory/" + name);
    flint::Platform p;
    p.build_id = "desktop-simulator-v1";
    p.memory = {48 * 1024, 0, 4096};
    p.monotonic = monotonic;
    p.free_storage = [] { return uint64_t(1024 * 1024); };
    core_ = std::make_unique<flint::Core>(
        (root + "/packages").toStdString(), (root + "/factory").toStdString(), p);
    auto error = core_->begin();
    if (error != flint::Error::ok) {
        fail(platformError(error));
        return;
    }
    session_ = {true, false, 1};
    handshake();
}
void PlatformClient::handshake() {
    status_ = "Проверка совместимости…";
    emit changed();
    flint::Writer w;
    w.n(flint::ProtocolVersion, 2);
    w.n(flint::ContractRevision);
    w.text("flint-flare-" FLINT_FLARE_VERSION);
    w.n(flint::CapPackages | flint::CapDeclarative, 8);
    request(flint::Method::hello, w.data, [this](auto error, auto bytes) {
        if (error != flint::Error::ok) {
            fail(platformError(error));
            return;
        }
        flint::Reader r{bytes};
        auto version = r.n(2), revision = r.n();
        auto build = r.text(96), profile = r.text(96);
        auto caps = r.n(8);
        if (!r.done() || profile != flint::DeviceProfile ||
            flint::compatible({uint16_t(version), uint32_t(revision), build, caps},
                flint::CapPackages | flint::CapDeclarative) != flint::Error::ok) {
            fail("Несовместимый контракт платформы");
            return;
        }
        capabilities_ = caps;
        connected_ = true;
        status_ = core_ ? "Локальный симулятор" : "Подключено";
        emit changed();
        refreshInfo();
        emit ready();
        if (autoSyncTime())
            syncTime();
    });
}
void PlatformClient::request(flint::Method method, flint::Bytes payload, Callback callback) {
    if (!core_ && !serial_.isOpen() && !(ble_ && ble_->ready())) {
        callback(flint::Error::source_unavailable, {});
        return;
    }
    if (queue_.size() > 64) {
        callback(flint::Error::quota_exceeded, {});
        return;
    }
    if (++nextId_ == 0)
        ++nextId_;
    queue_.push_back({{method, nextId_, false, std::move(payload)}, std::move(callback)});
    sendNext();
}
void PlatformClient::sendNext() {
    if (pending_ || queue_.empty())
        return;
    pending_ = std::make_unique<Pending>(std::move(queue_.front()));
    queue_.pop_front();
    auto wire = flint::encodeFrame(pending_->frame);
    if (wire.empty()) {
        fail("Запрос превышает лимит протокола");
        return;
    }
    timeout_.start();
    if (core_) {
        QTimer::singleShot(
            0, this, [this, wire, generation = generation_, id = pending_->frame.id] {
                if (generation != generation_ || !core_ || !pending_ || pending_->frame.id != id)
                    return;
                flint::FrameParser parser;
                parser.feed(wire, [this](flint::Frame frame) {
                    auto reply = flint::encodeFrame(core_->handle(frame, session_));
                    receive(QByteArray(reinterpret_cast<const char *>(reply.data()), reply.size()));
                });
            });
    } else if (serial_.isOpen() &&
        (pending_->frame.method == flint::Method::pairing || !(ble_ && ble_->ready()))) {
        if (serial_.write(reinterpret_cast<const char *>(wire.data()), wire.size()) !=
            qint64(wire.size())) {
            fail("Не удалось отправить USB-пакет");
        }
    } else if (!ble_->write(QByteArray(reinterpret_cast<const char *>(wire.data()), wire.size()))) {
        fail("Не удалось отправить BLE-пакет");
    }
}
void PlatformClient::receive(const QByteArray &bytes) {
    parser_.feed({reinterpret_cast<const uint8_t *>(bytes.data()), size_t(bytes.size())},
        [this](flint::Frame frame) {
            if (!pending_ || !frame.reply || frame.id != pending_->frame.id ||
                frame.method != pending_->frame.method)
                return;
            timeout_.stop();
            auto done = std::move(pending_);
            flint::Reader r{frame.payload};
            auto error = flint::Error(r.n(2));
            if (!r.valid) {
                fail("Повреждённый ответ");
                return;
            }
            flint::Bytes data(frame.payload.begin() + 2, frame.payload.end());
            done->callback(error, std::move(data));
            QTimer::singleShot(0, this, [this] { sendNext(); });
        });
}
void PlatformClient::refreshInfo() {
    request(flint::Method::info, {}, [this](auto error, auto data) {
        if (error != flint::Error::ok) {
            emit failure(platformError(error));
            return;
        }
        flint::Reader r{data};
        QVariantMap info;
        info["profile"] = QString::fromStdString(r.text(96));
        info["build"] = QString::fromStdString(r.text(96));
        info["internal"] = qulonglong(r.n());
        info["external"] = qulonglong(r.n());
        info["storage"] = qulonglong(r.n(8));
        info["active"] = QString::fromStdString(r.text(64));
        info["component"] = QString::fromStdString(r.text(64));
        info["epoch"] = qulonglong(r.n(8));
        info["generation"] = qulonglong(r.n(8));
        if (!r.done()) {
            emit failure("Некорректные сведения устройства");
            return;
        }
        info_ = info;
        emit changed();
    });
}
void PlatformClient::syncTime() {
    flint::Writer w;
    w.n(QDateTime::currentSecsSinceEpoch(), 8);
    request(flint::Method::time_set, w.data, [this](auto e, auto) {
        if (e != flint::Error::ok)
            emit failure(platformError(e));
        else
            refreshInfo();
    });
}
void PlatformClient::setOffset(const QString &minutes) {
    flint::Writer w;
    w.text("utc_offset");
    w.text(minutes.toStdString());
    request(flint::Method::settings_set, w.data, [this](auto e, auto) {
        if (e != flint::Error::ok)
            emit failure(platformError(e));
    });
}
void PlatformClient::recover() {
    request(flint::Method::recover, {}, [this](auto e, auto) {
        if (e != flint::Error::ok)
            emit failure(platformError(e));
        else
            refreshInfo();
    });
}

bool PlatformClient::autoSyncTime() const {
    return QSettings().value("connection/autoSyncTime", true).toBool();
}
void PlatformClient::setAutoSyncTime(bool enabled) {
    QSettings().setValue("connection/autoSyncTime", enabled);
    emit changed();
}
