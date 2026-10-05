#include "NativeTests.hpp"
#include "QrService.hpp"
#include <bb/data/JsonDataAccess>
#include <QVariantMap>
#include <QVariantList>
#include <QDir>
#include <QUuid>
#include <cstdio>
namespace brcode {
void runNativeTests(){
    QString folder="data/tests/"+QUuid::createUuid().toString();QDir().mkpath(folder);
    QVariantList results;QStringList fixtures;
    fixtures<<"ASCII 123"<<QString::fromUtf8("中文二维码")<<"a\nb\r\nc"<<"  leading and trailing  "<<"'\"<&>\\%_"<<"https://example.org/?a=1&b=%20"<<QString::fromUtf8("🙂🚀")<<" \t\n";
    QrService qr;
    for(int i=0;i<fixtures.size();++i){
        QVariantMap r;r["name"]=QString("roundtrip-%1").arg(i);r["utf8Bytes"]=fixtures[i].toUtf8().size();
        bool ok=qr.generate(fixtures[i]);r["generated"]=ok;r["error"]=qr.error();
        bool exported=ok&&qr.exportPng(folder+QString("/fixture-%1.png").arg(i));r["exportedAndReadBack"]=exported;
        r["status"]=(ok&&exported)?"PASS":"FAIL";results.append(r);
    }
    QVariantMap empty;empty["name"]="empty-reject-and-no-stale-image";empty["status"]=(!qr.generate("")&&!qr.valid())?"PASS":"FAIL";results.append(empty);
    QVariantMap excess;excess["name"]="over-capacity-no-truncation";excess["status"]=(!qr.generate(QString(10000,QChar('x')))&&!qr.valid())?"PASS":"FAIL";results.append(excess);
    QVariantMap nul;nul["name"]="NUL-rejected";nul["status"]=!qr.generate(QString(QChar(0)))?"PASS":"FAIL";results.append(nul);
    QVariantMap report;report["suite"]="BRCode native offline QR";report["results"]=results;report["outputDirectory"]=folder;
    report["camera"]="NOT TESTED";report["crossDevice"]="NOT TESTED";
    bb::data::JsonDataAccess json;json.save(report,folder+"/native-tests.json");
    foreach(const QVariant &entry,results){QVariantMap r=entry.toMap();std::fprintf(stderr,"BRCODE_TEST %s %s\n",r["name"].toString().toUtf8().constData(),r["status"].toString().toUtf8().constData());}
}
}
