
QT       += core gui widgets gui-private quick websockets quick-private
CONFIG += c++11

TARGET = fgqcanvas
TEMPLATE = app

SOURCES += main.cpp\
    WindowData.cpp \
    fgcanvasgroup.cpp \
    fgcanvaselement.cpp \
    fgcanvaspaintcontext.cpp \
    localprop.cpp \
    fgcanvaspath.cpp \
    fgcanvastext.cpp \
    fgqcanvasmap.cpp \
    fgqcanvasimage.cpp \
    fgqcanvasfontcache.cpp \
    fgqcanvasimageloader.cpp \
    canvasitem.cpp \
    canvasconnection.cpp \
    applicationcontroller.cpp \
    canvasdisplay.cpp \
    canvaspainteddisplay.cpp \
    jsonutils.cpp


HEADERS +=  \
    WindowData.h \
    fgcanvasgroup.h \
    fgcanvaselement.h \
    fgcanvaspaintcontext.h \
    localprop.h \
    fgcanvaspath.h \
    fgcanvastext.h \
    fgqcanvasmap.h \
    fgqcanvasimage.h \
    canvasconnection.h \
    applicationcontroller.h \
    canvasdisplay.h \
    canvasitem.h \
    fgqcanvasfontcache.h \
    fgqcanvasimageloader.h \
    canvaspainteddisplay.h \
    jsonutils.h

RESOURCES += \
    fgqcanvas_resources.qrc


OTHER_FILES += \
    qml/* \
    doc/* \
    config/*

#Q_XCODE_DEVELOPMENT_TEAM.name = DEVELOPMENT_TEAM
#Q_XCODE_DEVELOPMENT_TEAM.value = "James Turner"
#QMAKE_MAC_XCODE_SETTINGS += Q_XCODE_DEVELOPMENT_TEAM

  ios {
      QMAKE_INFO_PLIST = ios/Info.plist
  }
