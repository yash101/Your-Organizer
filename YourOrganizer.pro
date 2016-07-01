TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
LIBS += -pthread -lpthread
LIBS += -lssl -lcrypto
LIBS += -lboost_system -lboost_thread


SOURCES += main.cpp \
    tcpserver.cpp \

HEADERS += \
    exceptions.h \
    automaticmutex.h \
    tcpserver.h \
