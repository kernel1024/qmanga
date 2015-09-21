#include <QApplication>
#include <locale.h>
#include "mainwindow.h"
#include "global.h"

#ifdef WITH_OCR
tesseract::TessBaseAPI* ocr = NULL;
#endif

int main(int argc, char *argv[])
{
    setlocale (LC_NUMERIC, "C");
#ifdef WITH_OCR
    ocr = initializeOCR();
#endif

    qRegisterMetaType<QIntList>("QIntList");
    qRegisterMetaType<QImageHash>("QImageHash");
    qRegisterMetaType<SQLMangaEntry>("SQLMangaEntry");
    qRegisterMetaType<QByteHash>("QByteHash");
    qRegisterMetaType<QUuid>("QUuid");
    qRegisterMetaTypeStreamOperators<ZStrMap>("ZStrMap");

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    
    return a.exec();
}
