#-------------------------------------------------
#
# Project created by QtCreator 2012-03-19T07:25:52
#
#-------------------------------------------------

QT       += core gui sql

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
    bookmarkdlg.cpp \
    global.cpp \
    zmangamodel.cpp \
    zsearchtab.cpp \
    zsearchloader.cpp \
    zrarreader.cpp \
    zpdfreader.cpp \
    albumselectordlg.cpp \
    zmangacache.cpp

LIBS += -lquazip -lmagic

HEADERS  += mainwindow.h \
    zmangaview.h \
    zabstractreader.h \
    zglobal.h \
    zzipreader.h \
    zscrollarea.h \
    settingsdialog.h \
    bookmarkdlg.h \
    global.h \
    zmangamodel.h \
    zsearchtab.h \
    zsearchloader.h \
    zrarreader.h \
    zpdfreader.h \
    albumselectordlg.h \
    zmangacache.h

FORMS    += mainwindow.ui \
    settingsdialog.ui \
    bookmarkdlg.ui \
    zsearchtab.ui \
    albumselectordlg.ui

RESOURCES += \
    qmanga.qrc

CONFIG += warn_on link_pkgconfig use_kde_dialogs

MAGICK_CXX = $$system(Magick++-config --cxxflags)
MAGICK_LIBS = $$system(Magick++-config --libs)
QMAKE_CXXFLAGS += $$MAGICK_CXX
LIBS += $$MAGICK_LIBS

INCLUDEPATH += /usr/include/poppler/qt4
LIBS += -lpoppler-qt4

use_kde_dialogs {
    DEFINES += QB_KDEDIALOGS=1
    LIBS += -lkio -lkdecore -lkdeui
}
