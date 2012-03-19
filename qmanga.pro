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
    zmangaview.cpp

LIBS += -lquazip

HEADERS  += mainwindow.h \
    zmangaview.h

FORMS    += mainwindow.ui
