#-------------------------------------------------
#
# Project created by QtCreator 2012-03-19T07:25:52
#
#-------------------------------------------------

QT       += core gui widgets sql xml network

!win32 {
    QT += dbus
}

TEMPLATE = app

!versionAtLeast(QT_VERSION, 6.5.0):error("Use at least Qt version 6.5.0")

# warn on *any* usage of deprecated APIs
DEFINES += QT_DEPRECATED_WARNINGS
# ... and just fail to compile if APIs deprecated in Qt <= 6.5 are used
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060500

DEFINES += QT_NO_KEYWORDS
DEFINES += QT_NO_CAST_FROM_ASCII

SOURCES += main.cpp\
        mainwindow.cpp \
    ocr/abstractocr.cpp \
    ocr/googlevision.cpp \
    ocr/tesseractocr.cpp \
    readers/textdoc/zabstracttextdocument.cpp \
    readers/textdoc/zepubdocument.cpp \
    readers/textdoc/zfb2document.cpp \
    readers/textdoc/ztxtdocument.cpp \
    readers/ztextreader.cpp \
    zfilesystemscannermodel.cpp \
    zglobalprivate.cpp \
    zmangaview.cpp \
    zglobal.cpp \
    zscrollarea.cpp \
    settingsdialog.cpp \
    bookmarkdlg.cpp \
    global.cpp \
    zmangamodel.cpp \
    zsearchtab.cpp \
    albumselectordlg.cpp \
    zdb.cpp \
    zmangaloader.cpp \
    ocreditor.cpp \
    zexportdialog.cpp \
    zfilecopier.cpp \
    imagescale/scalefilter.cpp \
    multiinputdialog.cpp \
    readers/zdjvureader.cpp \
    readers/zabstractreader.cpp \
    readers/zzipreader.cpp \
    readers/zrarreader.cpp \
    readers/zpdfreader.cpp \
    readers/zsingleimagereader.cpp \
    readers/zimagesdirreader.cpp \
    ztextdocumentcontroller.cpp

HEADERS  += mainwindow.h \
    ocr/abstractocr.h \
    ocr/googlevision.h \
    ocr/tesseractocr.h \
    readers/textdoc/zabstracttextdocument.h \
    readers/textdoc/zepubdocument.h \
    readers/textdoc/zfb2document.h \
    readers/textdoc/ztxtdocument.h \
    readers/ztextreader.h \
    zfilesystemscannermodel.h \
    zglobalprivate.h \
    zmangaview.h \
    zglobal.h \
    zscrollarea.h \
    settingsdialog.h \
    bookmarkdlg.h \
    global.h \
    zmangamodel.h \
    zsearchtab.h \
    albumselectordlg.h \
    zdb.h \
    zmangaloader.h \
    ocreditor.h \
    zexportdialog.h \
    zfilecopier.h \
    imagescale/scalefilter.h \
    multiinputdialog.h \
    readers/zdjvureader.h \
    readers/zabstractreader.h \
    readers/zzipreader.h \
    readers/zrarreader.h \
    readers/zpdfreader.h \
    readers/zsingleimagereader.h \
    readers/zimagesdirreader.h \
    ztextdocumentcontroller.h

FORMS    += \
    settingsdialog.ui \
    bookmarkdlg.ui \
    zsearchtab.ui \
    albumselectordlg.ui \
    ocreditor.ui \
    zexportdialog.ui \
    multiinputdialog.ui \
    mainwindow.ui

RESOURCES += \
    qmanga.qrc

CONFIG += warn_on link_pkgconfig c++20

LIBS += -lz -ltbb

# for deprecations in GCC 11-12 without oneTBB
DEFINES += TBB_SUPPRESS_DEPRECATED_MESSAGES

message("Using Qt:            " $$QT_VERSION)

!win32 {
    PKGCONFIG += openssl

    packagesExist(libzip) {
        PKGCONFIG += libzip
    } else {
        error("Dependency error: libzip not found.")
    }

    packagesExist(icu-uc icu-i18n) {
        PKGCONFIG += icu-uc icu-i18n
    } else {
        error("Dependency error: icu not found.");
    }

    packagesExist(poppler-cpp) {
        packagesExist(poppler) {
            CONFIG += use_poppler
            message("Using Poppler:        YES")
        } else {
            message("Using Poppler:        NO")
        }
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

    exists(/usr/lib/libepub.so) {
        CONFIG += use_epub
        message("Using libepub:        YES")
    } else {
        exists(/usr/local/lib/libepub.so) {
            CONFIG += use_epub
            message("Using libepub:        YES")
        } else {
            message("Using libepub:        NO")
        }
    }

    use_poppler {
        DEFINES += WITH_POPPLER=1
        PKGCONFIG += poppler-cpp poppler
    }

    use_ocr {
        DEFINES += WITH_TESSERACT=1
        QMAKE_CXXFLAGS += -Wno-ignored-qualifiers
        PKGCONFIG += tesseract lept
    }

    use_djvu {
        DEFINES += WITH_DJVU=1
        PKGCONFIG += ddjvuapi
    }

    use_epub {
        DEFINES += WITH_EPUB=1
        LIBS += -lepub
    }

    DBUS_INTERFACES = org.kernel1024.jpreader.auxtranslator.xml \
        org.kernel1024.jpreader.browsercontroller.xml \
        org.qjrad.dictionary.xml
}

win32 {
    PKGCONFIG += libzip icu-uc icu-i18n openssl

    DEFINES += WITH_POPPLER=1
    PKGCONFIG += poppler

    DEFINES += WITH_TESSERACT=1
    QMAKE_CXXFLAGS += -Wno-ignored-qualifiers
    PKGCONFIG += tesseract
    LIBS += -llept

    DEFINES += WITH_DJVU=1
    PKGCONFIG += ddjvuapi

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

isEmpty(INSTALL_PREFIX):INSTALL_PREFIX = /usr
TARGET       = qmanga
TARGET.files = qmanga
TARGET.path  = $$INSTALL_PREFIX/bin
INSTALLS    += TARGET desktop icons

desktop.files	= qmanga.desktop
desktop.path	= $$INSTALL_PREFIX/share/applications

icons.files	= img/Alien9.png
icons.path	= $$INSTALL_PREFIX/share/icons
