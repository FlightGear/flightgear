#-------------------------------------------------
#
# Project created by QtCreator 2016-11-30T22:34:11
#
#-------------------------------------------------

QT       += core gui websockets
CONFIG += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = fgqcanvas
TEMPLATE = app


SOURCES += main.cpp\
        temporarywidget.cpp \
    fgcanvasgroup.cpp \
    fgcanvaselement.cpp \
    fgcanvaspaintcontext.cpp \
    localprop.cpp \
    fgcanvaswidget.cpp \
    fgcanvaspath.cpp \
    fgcanvastext.cpp \
    fgqcanvasmap.cpp \
    canvastreemodel.cpp \
    fgqcanvasimage.cpp

HEADERS  += temporarywidget.h \
    fgcanvasgroup.h \
    fgcanvaselement.h \
    fgcanvaspaintcontext.h \
    localprop.h \
    fgcanvaswidget.h \
    fgcanvaspath.h \
    fgcanvastext.h \
    fgqcanvasmap.h \
    canvastreemodel.h \
    fgqcanvasimage.h

FORMS    += temporarywidget.ui


Q_XCODE_DEVELOPMENT_TEAM.name = DEVELOPMENT_TEAM
Q_XCODE_DEVELOPMENT_TEAM.value = "James Turner"
QMAKE_MAC_XCODE_SETTINGS += Q_XCODE_DEVELOPMENT_TEAM


  ios {
      QMAKE_INFO_PLIST = ios/Info.plist
  }
