#-------------------------------------------------
#
# Project created by QtCreator 2012-03-19T07:25:52
#
#-------------------------------------------------

QT       += core gui

TARGET = qmanga
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    zmangaview.cpp \
    zabstractreader.cpp \
    zglobal.cpp \
    zzipreader.cpp

LIBS += -lquazip -lmagic

HEADERS  += mainwindow.h \
    zmangaview.h \
    zabstractreader.h \
    zglobal.h \
    zzipreader.h

FORMS    += mainwindow.ui

RESOURCES += \
    qmanga.qrc

CONFIG += warn_on link_pkgconfig use_kde_dialogs

use_kde_dialogs {
    DEFINES += QB_KDEDIALOGS=1
    LIBS += -lkio -lkdecore -lkdeui
}
