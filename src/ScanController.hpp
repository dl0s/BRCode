#ifndef BRCODE_SCANCONTROLLER_HPP
#define BRCODE_SCANCONTROLLER_HPP
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <bb/cascades/Container>
#include <bb/cascades/multimedia/Camera>
#include <bb/cascades/multimedia/BarcodeDetector>
#include "ScanGate.hpp"
namespace brcode {
class ScanController:public QObject {
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(bool scanning READ scanning NOTIFY changed)
    Q_PROPERTY(bool failed READ failed NOTIFY changed)
public:
    explicit ScanController(QObject *parent=0);
    ~ScanController();
    void attach(bb::cascades::Container *preview);
    void setPageVisible(bool visible);
    QString status()const{return status_;}
    bool scanning()const{return phase_==Running;}
    bool failed()const{return fault_;}
    Q_INVOKABLE void newSession();
public slots:
    void foreground();
    void background();
    void shutdown();
private slots:
    void opened();void started();void stopped();void closed();
    void detected(const QString &text,bb::cascades::multimedia::BarcodeFormat::Type format);
    void openFailed(bb::cascades::multimedia::CameraError::Type error);
    void previewFailed(bb::cascades::multimedia::CameraError::Type error);
    void stopFailed(bb::cascades::multimedia::CameraError::Type error);
    void closeFailed(bb::cascades::multimedia::CameraError::Type error);
    void timeout();
signals:
    void changed();
    void decoded(const QString &text);
private:
    enum Phase{Idle,Opening,Starting,Running,Stopping,Closing};
    bool wanted()const{return visible_&&foreground_&&!latched_&&!fault_&&!exiting_;}
    void reconcile();void stop();void release();void destroyCamera();
    void failure(const QString &action,bb::cascades::multimedia::CameraError::Type error);
    QPointer<bb::cascades::Container> preview_;
    QPointer<bb::cascades::multimedia::Camera> camera_;
    QPointer<bb::cascades::multimedia::BarcodeDetector> detector_;
    QTimer timer_;
    ScanGate gate_;
    Phase phase_;
    bool visible_,foreground_,latched_,fault_,exiting_,retiring_;
    QString status_;
};
}
#endif
