#ifndef BRCODE_QRSERVICE_HPP
#define BRCODE_QRSERVICE_HPP
#include <QImage>
#include <QString>
#include <bb/ImageData>
#include <bb/cascades/Image>
namespace brcode {
class QrService {
public:
    bool generate(const QString &text);
    void clear(){modules_=QImage();text_.clear();}
    bool valid()const{return !modules_.isNull();}
    QString text()const{return text_;}
    QString error()const{return error_;}
    int pixels(int bound)const;
    QImage raster(int bound)const;
    bb::cascades::Image image(int bound)const;
    bool exportPng(const QString &path);
    static bb::ImageData imageData(const QImage &image);
private:
    QImage modules_;
    QString text_,error_;
};
}
#endif
