TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
LIBS += -pthread -lpthread
LIBS += -lssl -lcrypto
LIBS += -lboost_system -lboost_thread
LIBS += -lpq


SOURCES += main.cpp \
    base/config.cpp \
    base/string_algorithms.cpp \
    database/database_basics.cpp \
    libraries/tinyxml2.cpp \
    server/http_internals.cpp \
    server/httpserver.cpp \
    server/tcpserver.cpp \
    server/tcpserver_internals.cpp \
    server/websockets.cpp \
    core/webserver.cpp \
    core/httprequesthandler.cpp

HEADERS += \
    base/automaticmutex.h \
    base/config.h \
    base/exceptions.h \
    base/string_algorithms.h \
    database/database_basics.h \
    libraries/tinyxml2.h \
    server/http_internals.h \
    server/httpserver.h \
    server/tcpserver.h \
    core/webserver.h \
    core/httprequesthandler.h

DISTFILES += \
    CONFIGURATION.md \
    LICENSE \
    README.md
