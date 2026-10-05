#ifndef BRCODE_APP_HPP
#define BRCODE_APP_HPP
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QPointer>
#include <bb/cascades/ArrayDataModel>
#include <bb/system/InvokeManager>
#include <bb/AbstractBpsEventHandler>
#include "HistoryStore.hpp"
#include "QrService.hpp"
#include "ScanController.hpp"
namespace brcode {
class App:public QObject,public bb::AbstractBpsEventHandler {
    Q_OBJECT
    Q_PROPERTY(int page READ page NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QString draft READ draft WRITE setDraft NOTIFY draftChanged)
    Q_PROPERTY(QVariantMap detail READ detail NOTIFY detailChanged)
    Q_PROPERTY(bool saved READ saved NOTIFY changed)
    Q_PROPERTY(bool pendingSave READ pendingSave NOTIFY changed)
    Q_PROPERTY(bool hasQr READ hasQr NOTIFY changed)
    Q_PROPERTY(QVariant qrImage READ qrImage NOTIFY changed)
    Q_PROPERTY(QVariant fullImage READ fullImage NOTIFY changed)
    Q_PROPERTY(int qrPixels READ qrPixels NOTIFY changed)
    Q_PROPERTY(int fullPixels READ fullPixels NOTIFY changed)
    Q_PROPERTY(bool hasMore READ hasMore NOTIFY changed)
    Q_PROPERTY(bool canOpenUrl READ canOpenUrl NOTIFY changed)
    Q_PROPERTY(bb::cascades::ArrayDataModel* historyModel READ historyModel CONSTANT)
public:
    explicit App(QObject *parent=0);
    ~App();
    ScanController *scanner(){return &scanner_;}
    void attachScene(QObject *scene){scene_=scene;}
    virtual void event(bps_event_t *event);
    int page()const{return page_;}
    QString message()const{return message_;}
    QString draft()const{return draft_;}
    void setDraft(const QString &text);
    QVariantMap detail()const;
    bool saved()const{return saved_;}
    bool pendingSave()const{return hasPending_&&!saved_;}
    bool hasQr()const{return qr_.valid()&&qr_.text()==draft_;}
    QVariant qrImage()const{return QVariant::fromValue(qr_.image(340));}
    QVariant fullImage()const{return QVariant::fromValue(qr_.image(560));}
    int qrPixels()const{return qr_.pixels(340);}
    int fullPixels()const{return qr_.pixels(560);}
    bool hasMore()const{return hasMore_;}
    bool canOpenUrl()const;
    bb::cascades::ArrayDataModel *historyModel(){return &history_;}
    Q_INVOKABLE void setPage(int page);
    Q_INVOKABLE void continueScan();
    Q_INVOKABLE bool generate(QObject *editor);
    Q_INVOKABLE bool retrySave();
    Q_INVOKABLE void search(const QString &text);
    Q_INVOKABLE void loadMore();
    Q_INVOKABLE void viewRecord(const QString &id);
    Q_INVOKABLE bool editNotes(const QString &title,const QString &notes);
    Q_INVOKABLE bool saveNotes(QObject *title,QObject *notes);
    Q_INVOKABLE bool deleteRecord();
    Q_INVOKABLE void regenerate();
    Q_INVOKABLE bool copyText(const QString &text);
    Q_INVOKABLE bool paste(QObject *editor);
    Q_INVOKABLE bool exportPng();
    Q_INVOKABLE void openUrl();
    Q_INVOKABLE void suspendInput();
public slots:
    void onScan(const QString &text);
    void foreground();void background();void shutdown();
    void invokeFinished();
signals:
    void changed();
    void detailChanged();
    void draftChanged();
    void draftReplaced(const QString &text);
private:
    bool savePending();
    bool ensureStore();
    void resetHistory();
    void setMessage(const QString &message);
    QUrl checkedUrl()const;
    bool allowedEditor(QObject *editor)const;
    QPointer<QObject> scene_;
    HistoryStore store_;
    QrService qr_;
    ScanController scanner_;
    bb::cascades::ArrayDataModel history_;
    bb::system::InvokeManager invoker_;
    Record current_;
    int page_;
    bool saved_,hasPending_,hasMore_,storeReady_,awake_,locked_;
    QString draft_,message_,search_,beforeTime_,beforeId_;
};
}
#endif
