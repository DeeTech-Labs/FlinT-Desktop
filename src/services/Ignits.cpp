#include "Ignits.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <cmath>
Ignits::Ignits(DeviceSession* session, QObject* parent) : QObject(parent), m_session(session) {
    for(const auto& name:{"mint-bundle","quiet-face","amber-theme"}) {
        QFile file(QString(":/ignits/%1.ignit").arg(name));
        if(file.open(QIODevice::ReadOnly))m_catalog.append(QJsonDocument::fromJson(file.readAll()).object().toVariantMap());
    }
    connect(session,&DeviceSession::stateChanged,this,[this] {
        if (!m_session->connected()) { m_request=0;m_installed.clear();m_storage.clear();m_state=tr("Disconnected"); }
        emit changed();
    });
    connect(session,&DeviceSession::sampleReceived,this,[this]{if(supported()&&!busy()&&m_storage.isEmpty())refresh();});
    connect(session,&DeviceSession::requestFinished,this,[this](int id,bool okay,const QVariant& value,const QString& error) {
        if(id!=m_request)return;
        m_request=0;
        if(!okay) m_state=tr("Device error: %1").arg(error);
        else if(m_operation=="IGNITSTORAGE") {m_storage=value.toMap();send("IGNITS");}
        else if(m_operation=="IGNITS") {m_installed=value.toList();m_state=tr("Installed Ignits");}
        else {m_state=tr("Confirmed by watch");emit packagesChanged();refresh();}
        emit changed();
    });
}
bool Ignits::supported() const {
    return m_session->connected() && m_session->info().value("capabilities").toList().contains("ignits_v1");
}
bool Ignits::encodePackage(const QByteArray& bytes,QString& wire,QString& error) {
    wire.clear();error=tr("Invalid or unsupported Flint Ignit");
    if(bytes.isEmpty()||bytes.size()>4096)return false;
    QJsonParseError parse;
    const auto document=QJsonDocument::fromJson(bytes,&parse);
    if(parse.error!=QJsonParseError::NoError||!document.isObject())return false;
    const auto p=document.object();
    const QSet<QString> keys{"schema","id","name","version","target","api","kind","layout","theme"};
    if(p.size()!=keys.size())return false;
    for(auto i=p.begin();i!=p.end();++i)if(!keys.contains(i.key()))return false;
    if(p.value("schema")!="flint.ignit/1"||p.value("target")!="flint_spark"||p.value("api")!=1)return false;
    if(!p.value("id").isString()||!QRegularExpression("^pkg\\.[a-z0-9.-]{1,20}$").match(p.value("id").toString()).hasMatch())return false;
    if(!p.value("name").isString())return false;
    const auto name=p.value("name").toString().trimmed();
    const auto utf8=name.toUtf8();
    if(utf8.isEmpty()||utf8.size()>24)return false;
    for(const auto c:name)if(c.isNonCharacter()||c.category()==QChar::Other_Control||c.category()==QChar::Other_Format)return false;
    const auto version=p.value("version");
    if(!version.isDouble()||!std::isfinite(version.toDouble())||version.toDouble()!=std::floor(version.toDouble())||version.toDouble()<1||version.toDouble()>65535)return false;
    const QStringList kinds{"face","theme","bundle"},layouts{"digital","blocks","quiet"};
    if(!p.value("kind").isString()||!p.value("layout").isString())return false;
    const int kind=kinds.indexOf(p.value("kind").toString()),layout=layouts.indexOf(p.value("layout").toString());
    if(kind<0||layout<0||!p.value("theme").isObject())return false;
    const auto theme=p.value("theme").toObject();
    const QStringList colors{"background","surface","accent","muted","text","selection"};
    if(theme.size()!=colors.size())return false;
    QStringList values{"1",p.value("id").toString(),QString::fromLatin1(utf8.toHex()),QString::number(version.toInt()),QString::number(kind),QString::number(layout)};
    for(const auto& key:colors) {
        if(!theme.value(key).isString()||!QRegularExpression("^#[0-9a-fA-F]{6}$").match(theme.value(key).toString()).hasMatch())return false;
        values.append(theme.value(key).toString().mid(1).toLower());
    }
    wire=values.join(' ');error.clear();return true;
}
void Ignits::send(const QString& operation,const QString& arguments) {
    if(!supported()||busy())return;
    m_request=m_session->request(operation,arguments);m_operation=operation;
    m_state=m_request?tr("Waiting for watch confirmation…"):tr("Cannot send to watch");emit changed();
}
void Ignits::install(const QUrl& url) {
    if(!supported()||busy())return;
    QFile file(url.toLocalFile());QString wire,error;
    if(!url.isLocalFile()||!file.open(QIODevice::ReadOnly)||file.size()>4096) {m_state=tr("Cannot read Ignit (limit 4 KB)");emit changed();return;}
    if(!encodePackage(file.readAll(),wire,error)){m_state=error;emit changed();return;}
    send("IGNITINSTALL",wire);
}
void Ignits::refresh(){send("IGNITSTORAGE");}
void Ignits::remove(const QString& id){if(QRegularExpression("^pkg\\.[a-z0-9.-]{1,20}$").match(id).hasMatch())send("IGNITREMOVE",id);}
void Ignits::useTheme(const QString& id){if(id.isEmpty()||QRegularExpression("^pkg\\.[a-z0-9.-]{1,20}$").match(id).hasMatch())send("IGNITTHEME",id);}
void Ignits::useFace(const QString& id){if(QRegularExpression("^pkg\\.[a-z0-9.-]{1,20}$").match(id).hasMatch())send("FACE",id);}

bool Ignits::canInstall(const QString& id) const {
    if(!supported()||busy()||m_storage.isEmpty()||m_storage.value("free_ram").toUInt()<m_storage.value("ram_reserve").toUInt())return false;
    bool update=false;
    for(const auto& p:m_installed)update|=p.toMap().value("id").toString()==id;
    return (update||m_storage.value("installed").toInt()<m_storage.value("capacity").toInt()) &&
        m_storage.value("free").toULongLong()>=m_storage.value("reserved").toULongLong()+m_storage.value("write_budget").toULongLong()+256;
}
void Ignits::installFromCatalog(const QString& id) {
    if(!canInstall(id))return;
    for(const auto& item:m_catalog)if(item.toMap().value("id").toString()==id) {
        QFile file(":/ignits/"+id.mid(4)+".ignit");QString wire,error;
        if(file.open(QIODevice::ReadOnly)&&encodePackage(file.readAll(),wire,error))send("IGNITINSTALL",wire);
        return;
    }
}
void Ignits::usePackage(const QString& id) {
    if(id=="digital"||QRegularExpression("^pkg\\.[a-z0-9.-]{1,20}$").match(id).hasMatch())send("IGNITUSE",id);
}
