#include <QtGui/QApplication>
#include "mainwindow.h"
#include "global.h"

int main(int argc, char *argv[])
{
    qRegisterMetaType<QIntList>("QIntList");
    qRegisterMetaType<QImageHash>("QImageHash");
    qRegisterMetaType<SQLMangaEntry>("SQLMangaEntry");
    qRegisterMetaType<QByteHash>("QByteHash");

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    
    return a.exec();
}
