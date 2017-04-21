
QT       += core gui gui-private quick websockets quick-private
CONFIG += c++11

!ios:!android
{
    greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

    SOURCES += canvastreemodel.cpp \
               elementdatamodel.cpp

    HEADERS += canvastreemodel.h \
               elementdatamodel.h

}

TARGET = fgqcanvas
TEMPLATE = app

SOURCES += main.cpp\
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
    canvasdisplay.cpp


HEADERS +=  \
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
    fgqcanvasimageloader.h

RESOURCES += \
    fgqcanvas_resources.qrc


OTHER_FILES += \
    qml/*


Q_XCODE_DEVELOPMENT_TEAM.name = DEVELOPMENT_TEAM
Q_XCODE_DEVELOPMENT_TEAM.value = "James Turner"
QMAKE_MAC_XCODE_SETTINGS += Q_XCODE_DEVELOPMENT_TEAM

  ios {
      QMAKE_INFO_PLIST = ios/Info.plist
  }
