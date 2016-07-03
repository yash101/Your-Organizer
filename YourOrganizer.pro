TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
LIBS += -pthread -lpthread
LIBS += -lssl -lcrypto
LIBS += -lboost_system -lboost_thread
LIBS += -lpq


SOURCES += main.cpp \
    tcpserver.cpp \
    tinyxml2.cpp \
    config.cpp \
    database/database_basics.cpp \
    httpserver.cpp \
    httpsupport.cpp

HEADERS += \
    exceptions.h \
    automaticmutex.h \
    tcpserver.h \
    tinyxml2.h \
    config.h \
    database/database_basics.h \
    httpserver.h \
    httpsupport.h

DISTFILES += \
    CONFIGURATION.md
