#include "ScanController.hpp"
#include <bb/cascades/HorizontalAlignment>
#include <bb/cascades/VerticalAlignment>
using namespace bb::cascades;
using namespace bb::cascades::multimedia;
namespace brcode {
ScanController::ScanController(QObject *p):QObject(p),phase_(Idle),visible_(false),foreground_(false),latched_(false),fault_(false),exiting_(false),retiring_(false),status_(QString::fromUtf8("相机尚未启动")){
    timer_.setSingleShot(true);connect(&timer_,SIGNAL(timeout()),this,SLOT(timeout()));
}
ScanController::~ScanController(){shutdown();destroyCamera();}
void ScanController::attach(Container *p){preview_=p;reconcile();}
void ScanController::setPageVisible(bool value){visible_=value;if(!value)gate_.stop();reconcile();}
void ScanController::foreground(){foreground_=true;reconcile();}
void ScanController::background(){foreground_=false;gate_.stop();reconcile();}
void ScanController::shutdown(){exiting_=true;gate_.stop();stop();}
void ScanController::newSession(){stop();latched_=false;fault_=false;gate_.stop();reconcile();}
void ScanController::reconcile(){
    if(!wanted()){stop();return;}
    if(phase_!=Idle||!preview_)return;
    camera_=new Camera();preview_->add(camera_);
    camera_->setHorizontalAlignment(HorizontalAlignment::Fill);camera_->setVerticalAlignment(VerticalAlignment::Fill);
    detector_=new BarcodeDetector(this);detector_->setFormats(BarcodeFormat::QrCode);detector_->setCamera(camera_);
    connect(camera_,SIGNAL(cameraOpened()),this,SLOT(opened()));
    connect(camera_,SIGNAL(cameraOpenFailed(bb::cascades::multimedia::CameraError::Type)),this,SLOT(openFailed(bb::cascades::multimedia::CameraError::Type)));
    connect(camera_,SIGNAL(viewfinderStarted()),this,SLOT(started()));
    connect(camera_,SIGNAL(viewfinderStartFailed(bb::cascades::multimedia::CameraError::Type)),this,SLOT(previewFailed(bb::cascades::multimedia::CameraError::Type)));
    connect(camera_,SIGNAL(viewfinderStopped()),this,SLOT(stopped()));
    connect(camera_,SIGNAL(viewfinderStopFailed(bb::cascades::multimedia::CameraError::Type)),this,SLOT(stopFailed(bb::cascades::multimedia::CameraError::Type)));
    connect(camera_,SIGNAL(cameraClosed()),this,SLOT(closed()));
    connect(camera_,SIGNAL(cameraCloseFailed(bb::cascades::multimedia::CameraError::Type)),this,SLOT(closeFailed(bb::cascades::multimedia::CameraError::Type)));
    connect(detector_,SIGNAL(detected(QString,bb::cascades::multimedia::BarcodeFormat::Type)),this,SLOT(detected(QString,bb::cascades::multimedia::BarcodeFormat::Type)));
    phase_=Opening;status_=QString::fromUtf8("正在打开后置相机…");timer_.start(10000);emit changed();camera_->open(CameraUnit::Rear);
}
void ScanController::opened(){if(sender()!=camera_||phase_!=Opening)return;if(!wanted()||retiring_){release();return;}phase_=Starting;status_=QString::fromUtf8("正在启动预览…");timer_.start(10000);emit changed();camera_->startViewfinder();}
void ScanController::started(){if(sender()!=camera_||phase_!=Starting)return;timer_.stop();phase_=Running;if(!wanted()||retiring_){stop();return;}gate_.begin();status_=QString::fromUtf8("对准二维码，识别后自动暂停");emit changed();}
void ScanController::stop(){
    gate_.stop();if(detector_)disconnect(detector_,0,this,0);
    if(!camera_||phase_==Idle)return;
    retiring_=true;
    if(phase_==Closing||phase_==Stopping)return;
    if(phase_==Opening||phase_==Starting)return; // Wait for asynchronous acknowledgement.
    phase_=Stopping;timer_.start(10000);camera_->stopViewfinder();emit changed();
}
void ScanController::stopped(){if(sender()!=camera_)return;release();}
void ScanController::release(){gate_.stop();if(!camera_)return;phase_=Closing;timer_.start(10000);camera_->close();emit changed();}
void ScanController::closed(){if(sender()!=camera_)return;destroyCamera();reconcile();}
void ScanController::destroyCamera(){
    timer_.stop();gate_.stop();
    if(detector_){detector_->setCamera(0);delete detector_;detector_=0;}
    if(camera_){disconnect(camera_,0,this,0);if(preview_)preview_->remove(camera_);camera_->deleteLater();camera_=0;}
    phase_=Idle;retiring_=false;if(!fault_)status_=latched_?QString::fromUtf8("识别完成；点击继续扫描开始新会话"):QString::fromUtf8("相机已释放");emit changed();
}
void ScanController::detected(const QString &text,BarcodeFormat::Type format){
    if(sender()!=detector_||phase_!=Running||retiring_||!wanted()||format!=BarcodeFormat::QrCode||!gate_.accept(gate_.epoch()))return;
    latched_=true;stop();status_=QString::fromUtf8("识别完成");emit changed();emit decoded(text);
}
void ScanController::failure(const QString &action,CameraError::Type error){
    gate_.stop();fault_=true;
    status_=error==CameraError::CameraInUse?QString::fromUtf8("相机被其他应用占用，请关闭占用应用后重试。"):
        QString::fromUtf8("%1失败（错误码 %2）。请检查相机授权，或关闭占用相机的应用后重试。").arg(action).arg(static_cast<int>(error));
    emit changed();
}
void ScanController::openFailed(CameraError::Type e){if(sender()!=camera_)return;failure(QString::fromUtf8("打开相机"),e);destroyCamera();}
void ScanController::previewFailed(CameraError::Type e){if(sender()!=camera_)return;failure(QString::fromUtf8("启动预览"),e);release();}
void ScanController::stopFailed(CameraError::Type e){if(sender()!=camera_)return;failure(QString::fromUtf8("停止预览"),e);release();}
void ScanController::closeFailed(CameraError::Type e){if(sender()!=camera_)return;failure(QString::fromUtf8("释放相机"),e);destroyCamera();}
void ScanController::timeout(){fault_=true;gate_.stop();status_=QString::fromUtf8("相机操作超时，请重试。离页后不接受旧帧。");emit changed();if(phase_==Closing)destroyCamera();else release();}
}
