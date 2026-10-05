#include "App.hpp"
#include <bb/cascades/Application>
#include <bb/cascades/TextArea>
#include <bb/cascades/TextField>
#include <bb/cascades/TextEditor>
#include <bb/system/Clipboard>
#include <bb/system/InvokeRequest>
#include <bb/system/InvokeTargetReply>
#include <bb/system/InvokeReplyError>
#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <QUuid>
#include <QRegExp>
#include <bps/navigator.h>
using namespace bb::cascades;
namespace brcode {
namespace {
std::string utf8(const QString &s){QByteArray a=s.toUtf8();return std::string(a.constData(),a.size());}
QString text(const std::string &s){return QString::fromUtf8(s.data(),s.size());}
QString now(){return QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddTHH:mm:ss.zzzZ");}
Record newRecord(const QString &body,const char *source){Record r;r.id=utf8(QUuid::createUuid().toString());r.source=source;r.body=utf8(body);r.created=utf8(now());r.updated=r.created;return r;}
}
App::App(QObject *p):QObject(p),history_(),page_(0),saved_(false),hasPending_(false),hasMore_(false),storeReady_(false),awake_(true),locked_(true){
    connect(&scanner_,SIGNAL(decoded(QString)),this,SLOT(onScan(QString)));
    navigator_request_events(0);subscribe(navigator_get_domain());
    locked_=navigator_get_device_lock_state()!=NAVIGATOR_DEVICE_LOCK_STATE_UNLOCKED;
    ensureStore();resetHistory();
}
App::~App(){shutdown();}
bool App::ensureStore(){
    if(storeReady_)return true;
    if(!QDir().mkpath("data")){setMessage(QString::fromUtf8("无法创建私有存储目录；当前结果仍保留。"));return false;}
    storeReady_=store_.open(utf8(QDir::currentPath()+"/data/history.sqlite"));
    if(!storeReady_)setMessage(QString::fromUtf8("历史数据库不可用：%1；不会清空旧记录。").arg(text(store_.error())));
    return storeReady_;
}
void App::setMessage(const QString &s){message_=s;emit changed();}
QVariantMap App::detail()const{
    QVariantMap m;m["id"]=text(current_.id);m["source"]=text(current_.source);m["body"]=text(current_.body);
    m["title"]=text(current_.title);m["notes"]=text(current_.notes);m["created"]=text(current_.created);m["updated"]=text(current_.updated);return m;
}
void App::suspendInput(){
    if(!scene_)return;
    foreach(AbstractTextControl *field,scene_->findChildren<AbstractTextControl *>())
        if(field->isFocused())field->loseFocus();
}
void App::setPage(int p){
    if(p<0||p>3||p==page_)return;
    suspendInput();page_=p;scanner_.setPageVisible(p==0);
    if(p==2)resetHistory();emit changed();
}
void App::continueScan(){
    if(hasPending_&&!saved_){setMessage(QString::fromUtf8("当前结果保存失败，请先重试保存；正文仍在详情页。"));return;}
    setPage(0);scanner_.newSession();scanner_.setPageVisible(true);
}
void App::setDraft(const QString &value){if(draft_==value)return;draft_=value;qr_.clear();emit draftChanged();emit changed();}
bool App::savePending(){
    if(!hasPending_||saved_)return saved_;
    if(!ensureStore()){setMessage(QString::fromUtf8("保存失败；当前结果仍保留，可重试。"));return false;}
    if(!store_.save(current_)){
        setMessage(QString::fromUtf8("保存失败：%1；当前结果仍保留，可重试。").arg(text(store_.error())));return false;
    }
    saved_=true;hasPending_=false;setMessage(QString::fromUtf8("已保存到历史"));resetHistory();return true;
}
bool App::retrySave(){return savePending();}
void App::onScan(const QString &s){
    if(page_!=0)return;
    if(hasPending_&&!saved_){setMessage(QString::fromUtf8("上次结果尚未保存，请重试。"));return;}
    current_=newRecord(s,"scan");saved_=false;hasPending_=true;emit detailChanged();setPage(3);savePending();emit changed();
}
bool App::generate(QObject *editor){
    if(hasPending_&&!saved_){setMessage(QString::fromUtf8("上次结果保存失败，请在详情重试保存后再生成。"));return false;}
    suspendInput();
    // The system input method commits through the native control on focus loss.
    if(editor)setDraft(editor->property("text").toString());
    if(!qr_.generate(draft_)){setMessage(qr_.error());return false;}
    current_=newRecord(draft_,"generate");saved_=false;hasPending_=true;emit detailChanged();
    bool saved=savePending();emit changed();return saved;
}
void App::search(const QString &s){search_=s;resetHistory();}
void App::resetHistory(){history_.clear();beforeTime_.clear();beforeId_.clear();hasMore_=true;loadMore();}
void App::loadMore(){
    if(!hasMore_||!ensureStore())return;
    std::vector<Record> rows;
    if(!store_.page(utf8(search_),utf8(beforeTime_),utf8(beforeId_),40,rows)){setMessage(QString::fromUtf8("读取历史失败：%1").arg(text(store_.error())));return;}
    foreach(const Record &r,rows){
        QVariantMap m;m["id"]=text(r.id);m["title"]=r.title.empty()?QString::fromUtf8(r.source=="scan"?"扫描记录":"生成记录"):text(r.title);
        m["summary"]=text(r.body).left(96).replace('\n',QString::fromUtf8(" ↵ "));m["created"]=text(r.created);
        history_.append(m);beforeTime_=text(r.created);beforeId_=text(r.id);
    }
    hasMore_=rows.size()==40;emit changed();
}
void App::viewRecord(const QString &id){
    if(hasPending_&&!saved_){setMessage(QString::fromUtf8("当前结果尚未保存，请先重试保存。"));return;}
    if(!ensureStore()||!store_.get(utf8(id),current_)){setMessage(QString::fromUtf8("无法读取这条记录：%1").arg(text(store_.error())));return;}
    saved_=true;hasPending_=false;emit detailChanged();setPage(3);emit changed();
}
bool App::editNotes(const QString &title,const QString &notes){
    if(!saved_){current_.title=utf8(title);current_.notes=utf8(notes);current_.updated=utf8(now());return savePending();}
    if(!ensureStore()||!store_.updateNotes(current_.id,utf8(title),utf8(notes),utf8(now()))){setMessage(QString::fromUtf8("备注保存失败：%1").arg(text(store_.error())));return false;}
    current_.title=utf8(title);current_.notes=utf8(notes);current_.updated=utf8(now());emit detailChanged();setMessage(QString::fromUtf8("标题和备注已保存"));resetHistory();return true;
}
bool App::deleteRecord(){
    if(!saved_)return false;
    if(!ensureStore()||!store_.remove(current_.id)){setMessage(QString::fromUtf8("删除失败：%1").arg(text(store_.error())));return false;}
    current_=Record();saved_=false;hasPending_=false;setMessage(QString::fromUtf8("记录已删除"));setPage(2);return true;
}
bool App::saveNotes(QObject *title,QObject *notes){
    if(!title||!notes||page_!=3)return false;
    suspendInput();
    QString nextTitle=title->property("text").toString(),nextNotes=notes->property("text").toString();
    return editNotes(nextTitle,nextNotes);
}
void App::regenerate(){setDraft(text(current_.body));setPage(1);emit draftReplaced(draft_);setMessage(QString::fromUtf8("已复制原文到编辑副本；点击生成将保存新记录。"));}
bool App::copyText(const QString &s){bb::system::Clipboard cb;if(!cb.clear()||!cb.insert("text/plain",s.toUtf8())){setMessage(QString::fromUtf8("复制失败，请重试。"));return false;}setMessage(QString::fromUtf8("已复制完整原文"));return true;}
bool App::paste(QObject *editor){
    TextArea *area=qobject_cast<TextArea *>(editor);TextField *field=qobject_cast<TextField *>(editor);
    if(!allowedEditor(editor)||(!area&&!field))return false;
    suspendInput();
    bb::system::Clipboard cb;QByteArray data=cb.value("text/plain");QString pasted=QString::fromUtf8(data.constData(),data.size());
    if(pasted.toUtf8()!=data){setMessage(QString::fromUtf8("剪贴板不是有效 UTF-8 文本；未修改输入。"));return false;}
    AbstractTextControl *control=area?static_cast<AbstractTextControl *>(area):field;
    TextEditor *edit=area?area->editor():field->editor();
    if(field&&(pasted.contains('\n')||pasted.contains('\r'))){setMessage(QString::fromUtf8("单行字段不能粘贴换行；原输入保留。"));return false;}
    const int expectedLength=control->text().size()-edit->selectedText().size()+pasted.size();
    if(expectedLength>(area?area->maximumLength():field->maximumLength())){setMessage(QString::fromUtf8("粘贴超过输入框容量；未截断正文。"));return false;}
    edit->insertPlainText(pasted);
    const bool ok=control->text().size()==expectedLength;
    if(!ok)setMessage(QString::fromUtf8("系统输入框未完整接收粘贴文字，请核对后重试。"));
    control->requestFocus();return ok;
}
bool App::exportPng(){
    if(!hasQr()){setMessage(QString::fromUtf8("输入已变化，请重新生成后导出。"));return false;}
    QString folder=QDir::currentPath()+"/shared/photos/BRCode";
    if(!QDir().mkpath(folder)){setMessage(QString::fromUtf8("无法写入共享照片目录；请检查共享文件权限。"));return false;}
    QString file=folder+"/BRCode-"+QDateTime::currentDateTimeUtc().toString("yyyyMMdd-hhmmss-zzz")+"-"+QUuid::createUuid().toString().mid(1,8)+".png";
    if(!qr_.exportPng(file)){setMessage(qr_.error());return false;}
    setMessage(QString::fromUtf8("PNG 已保存：%1").arg(QFileInfo(file).canonicalFilePath()));return true;
}
QUrl App::checkedUrl()const{
    QString body=text(current_.body);
    if(body.contains(QRegExp("[\\s\\x0000-\\x001f\\x007f]")))return QUrl();
    QUrl url=QUrl::fromEncoded(body.toUtf8(),QUrl::StrictMode);
    if(!url.isValid()||(url.scheme().toLower()!="http"&&url.scheme().toLower()!="https")||url.host().isEmpty()||!url.userInfo().isEmpty())return QUrl();
    return url;
}
bool App::canOpenUrl()const{return !checkedUrl().isEmpty();}
void App::openUrl(){
    QUrl url=checkedUrl();if(url.isEmpty()){setMessage(QString::fromUtf8("这不是有效的 HTTP/HTTPS 链接。"));return;}
    suspendInput();scanner_.background();bb::system::InvokeRequest request;request.setTarget("sys.browser");request.setAction("bb.action.OPEN");request.setUri(url);
    bb::system::InvokeTargetReply *reply=invoker_.invoke(request);
    if(!reply){setMessage(QString::fromUtf8("无法打开浏览器，请复制链接。"));return;}
    connect(reply,SIGNAL(finished()),this,SLOT(invokeFinished()));
}
void App::invokeFinished(){bb::system::InvokeTargetReply *reply=qobject_cast<bb::system::InvokeTargetReply *>(sender());if(!reply)return;if(reply->error()!=bb::system::InvokeReplyError::None)setMessage(QString::fromUtf8("浏览器调用失败，请复制链接后打开。"));reply->deleteLater();}
bool App::allowedEditor(QObject *editor)const{
    if(!editor)return false;QString name=editor->objectName();
    return (page_==1&&name=="generatorText")||(page_==2&&name=="historySearch")||(page_==3&&(name=="recordTitle"||name=="recordNotes"));
}
void App::event(bps_event_t *event){
    if(bps_event_get_domain(event)!=navigator_get_domain()||bps_event_get_code(event)!=NAVIGATOR_DEVICE_LOCK_STATE)return;
    locked_=navigator_event_get_device_lock_state(event)!=NAVIGATOR_DEVICE_LOCK_STATE_UNLOCKED;
    if(locked_){suspendInput();scanner_.background();}
    else if(awake_&&bb::cascades::Application::instance()->isFullscreen())scanner_.foreground();
}
void App::foreground(){awake_=true;locked_=navigator_get_device_lock_state()!=NAVIGATOR_DEVICE_LOCK_STATE_UNLOCKED;if(!locked_&&bb::cascades::Application::instance()->isFullscreen())scanner_.foreground();else scanner_.background();}
void App::background(){awake_=false;suspendInput();scanner_.background();}
void App::shutdown(){suspendInput();scanner_.shutdown();store_.close();storeReady_=false;}
}
