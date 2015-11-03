#-------------------------------------------------
#
# Project created by QtCreator 2012-03-19T07:25:52
#
#-------------------------------------------------

QT       += core gui dbus widgets

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
    zimagesdirreader.cpp

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
    zimagesdirreader.h

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

exists( /usr/include/magic.h ) {
    LIBS += -lmagic
} else {
    error("Dependency error: libmagic not found.")
}

packagesExist(zziplib) {
    PKGCONFIG += zziplib
} else {
    error("Dependency error: zziplib not found.")
}

packagesExist(poppler-cpp) {
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

system(Magick++-config --version > /dev/null) {
    CONFIG += use_magick
    message("Using ImageMagick:    YES")
} else {
    message("Using ImageMagick:    NO")
}

use_magick {
    DEFINES += WITH_MAGICK=1
    MAGICK_CXX = $$system(Magick++-config --cxxflags | sed 's/^.*-I/-I/')
    MAGICK_LIBS = $$system(Magick++-config --libs)
    QMAKE_CXXFLAGS += $$MAGICK_CXX
    LIBS += $$MAGICK_LIBS
}

use_poppler {
    DEFINES += WITH_POPPLER=1
    PKGCONFIG += poppler-cpp
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

OTHER_FILES += \
    org.jpreader.auxtranslator.xml

DBUS_INTERFACES = org.jpreader.auxtranslator.xml

# Workaround for _FORTIFY_SOURCE and required -O level
debug {
    QMAKE_CXXFLAGS += -O1
}
