#-------------------------------------------------
#
# Project created by QtCreator 2012-03-19T07:25:52
#
#-------------------------------------------------

QT       += core gui widgets sql concurrent

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
    scalefilter.cpp \
    multiinputdialog.cpp \
    zdjvureader.cpp

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
    scalefilter.h \
    multiinputdialog.h \
    zdjvureader.h

FORMS    += mainwindow.ui \
    settingsdialog.ui \
    bookmarkdlg.ui \
    zsearchtab.ui \
    albumselectordlg.ui \
    ocreditor.ui \
    zexportdialog.ui \
    multiinputdialog.ui

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

    packagesExist(ddjvuapi) {
        CONFIG += use_djvu
        message("Using DjVuLibre:      YES")
    } else {
        message("Using DjVuLibre:      NO")
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

    use_djvu {
        DEFINES += WITH_DJVU=1
        PKGCONFIG += ddjvuapi
    }

    DBUS_INTERFACES = org.kernel1024.jpreader.auxtranslator.xml \
        org.kernel1024.jpreader.browsercontroller.xml \
        org.qjrad.dictionary.xml
}

win32 {
    PKGCONFIG += zziplib

    DEFINES += WITH_POPPLER=1
    PKGCONFIG += poppler

    DEFINES += WITH_OCR=1
    QMAKE_CXXFLAGS += -Wno-ignored-qualifiers
    PKGCONFIG += tesseract
    LIBS += -llept

    RC_FILE = qmanga.rc
}

CONFIG(release, debug|release) {
    CONFIG += optimize_full
}

include( miniqxt/miniqxt.pri )

OTHER_FILES += \
    org.kernel1024.jpreader.auxtranslator.xml \
    org.kernel1024.jpreader.browsercontroller.xml \
    org.qjrad.dictionary.xml \
    README.md
