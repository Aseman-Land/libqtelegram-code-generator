#-------------------------------------------------
#
# Project created by QtCreator 2015-07-17T13:14:02
#
#-------------------------------------------------

QT       += core
QT       -= gui

TARGET = libqtelegram-generator

TEMPLATE = app


SOURCES += main.cpp \
    typegenerator.cpp \
    functiongenerator.cpp \
    abstractgenerator.cpp

HEADERS += \
    typegenerator.h \
    functiongenerator.h \
    abstractgenerator.h

RESOURCES += \
    resource.qrc
