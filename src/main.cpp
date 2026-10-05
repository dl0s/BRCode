#include "App.hpp"
#include <bb/cascades/Application>
#include <bb/cascades/QmlDocument>
#include <bb/cascades/Page>
#include <bb/cascades/Container>
#include <bb/data/JsonDataAccess>
#include <QDir>
#include <QVariantList>
#include <cstdio>
#include <unistd.h>
#ifdef BRCODE_BACKEND_TESTS
#include "../tests/NativeTests.hpp"
#endif
using namespace bb::cascades;
int main(int argc,char **argv){
    Application application(argc,argv);
    application.setMenuEnabled(false);
    brcode::App backend;
    QObject::connect(&application,SIGNAL(fullscreen()),&backend,SLOT(foreground()));
    QObject::connect(&application,SIGNAL(awake()),&backend,SLOT(foreground()));
    QObject::connect(&application,SIGNAL(thumbnail()),&backend,SLOT(background()));
    QObject::connect(&application,SIGNAL(invisible()),&backend,SLOT(background()));
    QObject::connect(&application,SIGNAL(asleep()),&backend,SLOT(background()));
    QObject::connect(&application,SIGNAL(aboutToQuit()),&backend,SLOT(shutdown()));
    QmlDocument *document=QmlDocument::create("asset:///main.qml").parent(&application);
    document->setContextProperty("app",&backend);document->setContextProperty("scanner",backend.scanner());
    Page *page=document->createRootObject<Page>();
    QVariantMap runtime;runtime["version"]="1.0.0.3";runtime["sceneReady"]=page!=0;runtime["uid"]=static_cast<int>(getuid());runtime["pid"]=static_cast<int>(getpid());runtime["inputMethod"]="system";
    QVariantList errors;foreach(const QDeclarativeError &error,document->errors()){errors.append(error.toString());std::fprintf(stderr,"%s\n",error.toString().toUtf8().constData());}runtime["qmlErrors"]=errors;
    bb::data::JsonDataAccess json;json.save(runtime,"data/runtime.json");
    if(!page)return 2;
    backend.attachScene(page);
    backend.scanner()->attach(page->findChild<Container *>("cameraPreview"));backend.scanner()->setPageVisible(true);
    application.setScene(page);if(application.isFullscreen())backend.foreground();
    std::fprintf(stderr,"BRCODE_SCENE_READY version=1.0.0.3 pid=%d uid=%d input=system qmlErrors=%d\n",static_cast<int>(getpid()),static_cast<int>(getuid()),errors.size());std::fflush(stderr);
#ifdef BRCODE_BACKEND_TESTS
    brcode::runNativeTests();
#endif
    return Application::exec();
}
