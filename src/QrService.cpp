#include "QrService.hpp"
#include <bb/PixelFormat>
#include <bb/cascades/multimedia/BarcodeGenerator>
#include <bb/cascades/multimedia/BarcodeGeneratorFormat>
#include <bb/cascades/multimedia/BarcodeGeneratorError>
#include <bb/cascades/multimedia/BarcodeDetector>
#include <bb/cascades/multimedia/BarcodeFormat>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>
using namespace bb::cascades::multimedia;
namespace brcode {
bb::ImageData QrService::imageData(const QImage &im){
    bb::ImageData data(bb::PixelFormat::RGBA_Premultiplied,im.width(),im.height());
    if(!data.isValid())return data;
    for(int y=0;y<im.height();++y){
        unsigned char *out=data.pixels()+y*data.bytesPerLine();
        for(int x=0;x<im.width();++x){QRgb p=im.pixel(x,y);out[x*4]=qRed(p);out[x*4+1]=qGreen(p);out[x*4+2]=qBlue(p);out[x*4+3]=255;}
    }return data;
}
bool QrService::generate(const QString &text){
    clear();error_.clear();
    if(text.isEmpty()){error_=QString::fromUtf8("请输入文字；空字符串不能生成二维码。");return false;}
    for(int i=0;i<text.size();++i){
        ushort c=text.at(i).unicode();
        if(c==0||(c>=0xDC00&&c<=0xDFFF)||(c>=0xD800&&c<=0xDBFF&&(i+1>=text.size()||text.at(i+1).unicode()<0xDC00||text.at(i+1).unicode()>0xDFFF))){
            error_=QString::fromUtf8("包含不支持的 NUL 或无效 Unicode 字符；未修改正文。");return false;
        }if(c>=0xD800&&c<=0xDBFF)++i;
    }
    BarcodeGenerator encoder;
    const bb::ImageData native=encoder.generate(BarcodeGeneratorFormat::QrCode,text,1.0f);
    if(!native.isValid()||encoder.error()!=BarcodeGeneratorError::None){
        if(encoder.error()==BarcodeGeneratorError::DataTooLong)
            error_=QString::fromUtf8("正文超过二维码容量（UTF-8 %1 字节）；请缩短后重试。未截断正文。").arg(text.toUtf8().size());
        else error_=QString::fromUtf8("原生编码器无法生成此正文，错误码 %1；未保存记录。").arg(static_cast<int>(encoder.error()));
        return false;
    }
    // Native API promises RGBA. Find the square symbol and its integer module pitch.
    int minx=native.width(),miny=native.height(),maxx=-1,maxy=-1;
    for(int y=0;y<native.height();++y){const unsigned char *p=native.constPixels()+y*native.bytesPerLine();
        for(int x=0;x<native.width();++x)if(p[4*x+3]>127&&p[4*x]<128&&p[4*x+1]<128&&p[4*x+2]<128){minx=qMin(minx,x);miny=qMin(miny,y);maxx=qMax(maxx,x);maxy=qMax(maxy,y);}}
    int run=0;
    if(maxx>=minx&&maxy>=miny){const unsigned char *row=native.constPixels()+miny*native.bytesPerLine();
        while(minx+run<=maxx&&row[(minx+run)*4]<128&&row[(minx+run)*4+3]>127)++run;}
    int pitch=run/7,side=maxx-minx+1;
    if(run%7||pitch<1||side!=maxy-miny+1||side%pitch||side/pitch<21||side/pitch>177||(side/pitch-21)%4){
        error_=QString::fromUtf8("原生二维码图像结构无法验证；未显示旧图。");return false;
    }
    int n=side/pitch;
    modules_=QImage(n,n,QImage::Format_RGB32);
    for(int y=0;y<n;++y)for(int x=0;x<n;++x){
        const unsigned char *p=native.constPixels()+(miny+y*pitch+pitch/2)*native.bytesPerLine()+(minx+x*pitch+pitch/2)*4;
        modules_.setPixel(x,y,(p[0]<128&&p[3]>127)?qRgb(0,0,0):qRgb(255,255,255));
    }
    // Check the exact black/white image used for preview/export before declaring success.
    BarcodeDetector decoder;decoder.setFormats(BarcodeFormat::QrCode);
    BarcodeFormat::Type format=static_cast<BarcodeFormat::Type>(0);
    QString decoded=decoder.scanImage(imageData(raster(1024)),&format);
    if(format!=BarcodeFormat::QrCode||decoded!=text){
        clear();error_=QString::fromUtf8("原生编解码无法完整回读这段文字（可能为编码限制）；未保存或截断。");return false;
    }
    text_=text;return true;
}
int QrService::pixels(int bound)const{if(!valid())return 0;int side=modules_.width()+8;return side*qMax(1,bound/side);}
QImage QrService::raster(int bound)const{
    if(!valid())return QImage();int unit=qMax(1,bound/(modules_.width()+8)),side=pixels(bound);
    QImage out(side,side,QImage::Format_RGB32);out.fill(qRgb(255,255,255));
    for(int y=0;y<modules_.height();++y)for(int x=0;x<modules_.width();++x)if(qRed(modules_.pixel(x,y))==0)
        for(int dy=0;dy<unit;++dy){QRgb *row=reinterpret_cast<QRgb *>(out.scanLine((y+4)*unit+dy));for(int dx=0;dx<unit;++dx)row[(x+4)*unit+dx]=qRgb(0,0,0);}
    return out;
}
bb::cascades::Image QrService::image(int bound)const{return valid()?bb::cascades::Image(imageData(raster(bound))):bb::cascades::Image();}
bool QrService::exportPng(const QString &path){
    if(!valid()){error_=QString::fromUtf8("请先成功生成当前文字的二维码。");return false;}
    if(QFileInfo(path).exists()){error_=QString::fromUtf8("导出文件已存在，请换一个文件名。");return false;}
    QTemporaryFile temporary(QFileInfo(path).absolutePath()+"/.brcode-XXXXXX.png");
    if(!temporary.open()){error_=QString::fromUtf8("不能写入导出目录：%1（%2）").arg(path,temporary.errorString());return false;}
    QString tempPath=temporary.fileName();temporary.close();
    if(!raster(1024).save(tempPath,"PNG")){error_=QString::fromUtf8("PNG 写入失败：%1").arg(path);return false;}
    QImage saved(tempPath);BarcodeDetector decoder;decoder.setFormats(BarcodeFormat::QrCode);
    if(saved.isNull()||decoder.scanImage(imageData(saved))!=text_){error_=QString::fromUtf8("导出的 PNG 回读失败；请重试。");return false;}
    if(!QFile::rename(tempPath,path)){error_=QString::fromUtf8("导出文件发布失败：%1").arg(path);return false;}
    return true;
}
}
