#include <QtGui/QApplication>
#include <locale.h>
#include "mainwindow.h"
#include "global.h"

tesseract::TessBaseAPI* ocr = NULL;

int main(int argc, char *argv[])
{
    setlocale (LC_NUMERIC, "C");
    ocr = initializeOCR();

    qRegisterMetaType<QIntList>("QIntList");
    qRegisterMetaType<QImageHash>("QImageHash");
    qRegisterMetaType<SQLMangaEntry>("SQLMangaEntry");
    qRegisterMetaType<QByteHash>("QByteHash");
    qRegisterMetaType<QUuid>("QUuid");

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    
    return a.exec();
}
