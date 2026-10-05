TEMPLATE = app
TARGET = brcode
CONFIG += qt warn_on cascades
QT += core gui declarative
QMAKE_CXXFLAGS += -std=c++98
INCLUDEPATH += src
SOURCES += src/main.cpp src/App.cpp src/HistoryStore.cpp src/QrService.cpp src/ScanController.cpp
HEADERS += src/App.hpp src/HistoryStore.hpp src/QrService.hpp src/ScanController.hpp src/ScanGate.hpp
LIBS += -lbbcascadesmultimedia -lbbdata -lbbsystem -lbps -lsqlite3
