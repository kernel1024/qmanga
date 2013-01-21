#-------------------------------------------------
#
# Project created by QtCreator 2012-03-19T07:25:52
#
#-------------------------------------------------

QT       += core gui sql dbus

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
    zrarreader.cpp \
    zpdfreader.cpp \
    albumselectordlg.cpp \
    zdb.cpp \
    zmangaloader.cpp \
    ocreditor.cpp

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
    zrarreader.h \
    zpdfreader.h \
    albumselectordlg.h \
    zdb.h \
    zmangaloader.h \
    ocreditor.h

FORMS    += mainwindow.ui \
    settingsdialog.ui \
    bookmarkdlg.ui \
    zsearchtab.ui \
    albumselectordlg.ui \
    ocreditor.ui

RESOURCES += \
    qmanga.qrc

CONFIG += warn_on link_pkgconfig use_magick use_poppler use_ocr

use_magick {
    DEFINES += WITH_MAGICK=1
    MAGICK_CXX = $$system(Magick++-config --cxxflags)
    MAGICK_LIBS = $$system(Magick++-config --libs)
    QMAKE_CXXFLAGS += $$MAGICK_CXX
    LIBS += $$MAGICK_LIBS
}

use_poppler {
    DEFINES += WITH_POPPLER=1
    INCLUDEPATH += /usr/include/poppler/qt4
    LIBS += -lpoppler-qt4
}

use_kde_dialogs {
    DEFINES += WITH_KDEDIALOGS=1
    LIBS += -lkio -lkdecore -lkdeui
}

use_ocr {
    DEFINES += WITH_OCR=1
    QMAKE_CXXFLAGS += -Wno-ignored-qualifiers
    INCLUDEPATH += /usr/include/tesseract
    LIBS += -llept -ltesseract
}

OTHER_FILES += \
    org.jpreader.auxtranslator.xml

DBUS_INTERFACES = org.jpreader.auxtranslator.xml
