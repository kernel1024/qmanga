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
    zzipreader.cpp \
    zscrollarea.cpp \
    settingsdialog.cpp \
    bookmarkdlg.cpp

LIBS += -lquazip -lmagic

HEADERS  += mainwindow.h \
    zmangaview.h \
    zabstractreader.h \
    zglobal.h \
    zzipreader.h \
    zscrollarea.h \
    settingsdialog.h \
    bookmarkdlg.h

FORMS    += mainwindow.ui \
    settingsdialog.ui \
    bookmarkdlg.ui

RESOURCES += \
    qmanga.qrc

CONFIG += warn_on link_pkgconfig use_kde_dialogs

MAGICK_CXX = $$system(Magick++-config --cxxflags)
MAGICK_LIBS = $$system(Magick++-config --libs)
QMAKE_CXXFLAGS += $$MAGICK_CXX
LIBS += $$MAGICK_LIBS

use_kde_dialogs {
    DEFINES += QB_KDEDIALOGS=1
    LIBS += -lkio -lkdecore -lkdeui
}
