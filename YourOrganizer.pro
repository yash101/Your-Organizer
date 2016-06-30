TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
LIBS += -pthread -lpthread
LIBS += -lssl -lcrypto


SOURCES += main.cpp \
    tcpsockets.cpp \
    tcpconnection.cpp

HEADERS += \
    tcpsockets.h \
    exceptions.h \
    automaticmutex.h \
    tcpconnection.h
