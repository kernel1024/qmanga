#-------------------------------------------------
#
# Project created by QtCreator 2012-03-19T07:25:52
#
#-------------------------------------------------

QT       += core gui widgets concurrent

!win32 {
    QT += dbus
}

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
    ocreditor.cpp \
    zsingleimagereader.cpp \
    zexportdialog.cpp \
    zimagesdirreader.cpp \
    zpdfimageoutdev.cpp \
    zfilecopier.cpp \
    scalefilter.cpp

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
    ocreditor.h \
    zsingleimagereader.h \
    zexportdialog.h \
    zimagesdirreader.h \
    zpdfimageoutdev.h \
    zfilecopier.h \
    scalefilter.h

FORMS    += mainwindow.ui \
    settingsdialog.ui \
    bookmarkdlg.ui \
    zsearchtab.ui \
    albumselectordlg.ui \
    ocreditor.ui \
    zexportdialog.ui

RESOURCES += \
    qmanga.qrc

CONFIG += warn_on link_pkgconfig c++11

!win32 {
    exists( /usr/include/magic.h ) {
        LIBS += -lmagic
        DEFINES += WITH_LIBMAGIC=1
    } else {
        error("Dependency error: libmagic not found.")
    }

    packagesExist(zziplib) {
        PKGCONFIG += zziplib
    } else {
        error("Dependency error: zziplib not found.")
    }

    packagesExist(poppler) {
        CONFIG += use_poppler
        message("Using Poppler:        YES")
    } else {
        message("Using Poppler:        NO")
    }

    packagesExist(tesseract) {
        CONFIG += use_ocr
        message("Using Tesseract OCR:  YES")
    } else {
        message("Using Tesseract OCR:  NO")
    }

    use_poppler {
        DEFINES += WITH_POPPLER=1
        PKGCONFIG += poppler
    }

    use_ocr {
        DEFINES += WITH_OCR=1
        QMAKE_CXXFLAGS += -Wno-ignored-qualifiers
        PKGCONFIG += tesseract
        LIBS += -llept
    }

    MYSQL_CXX = $$system(mysql_config --cflags)
    MYSQL_LIBS = $$system(mysql_config --libs)
    MYSQL_INC = $$system(mysql_config --include)
    QMAKE_CXXFLAGS += $$MYSQL_CXX
    LIBS += $$MYSQL_LIBS
    INCLUDEPATH += $$MYSQL_INC

    DBUS_INTERFACES = org.jpreader.auxtranslator.xml \
        org.qjrad.dictionary.xml
}

win32 {
    PKGCONFIG += zziplib

    DEFINES += WITH_POPPLER=1
    PKGCONFIG += poppler

    MYSQL_INC = -I$$(SYSROOT)/include/mysql
    MYSQL_LIBS = -L$$(SYSROOT)/lib  -lmysqlclient -lpthread -lz -lm
    LIBS += $$MYSQL_LIBS
    INCLUDEPATH += $$MYSQL_INC

    RC_FILE = qmanga.rc
}

CONFIG(release, debug|release) {
    CONFIG += optimize_full
}

include( miniqxt/miniqxt.pri )

OTHER_FILES += \
    org.jpreader.auxtranslator.xml \
    org.qjrad.dictionary.xml
