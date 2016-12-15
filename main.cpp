#include <QApplication>
#include <QDir>
#include <locale.h>
#include "mainwindow.h"
#include "global.h"
#ifdef _WIN32
#include <windows.h>
#endif

#ifdef WITH_OCR
tesseract::TessBaseAPI* ocr = NULL;
#endif

int main(int argc, char *argv[])
{
    setlocale (LC_NUMERIC, "C");
#ifdef WITH_OCR
    ocr = initializeOCR();
#endif
    qInstallMessageHandler(stdConsoleOutput);
    qRegisterMetaType<QIntList>("QIntList");
    qRegisterMetaType<QImageHash>("QImageHash");
    qRegisterMetaType<SQLMangaEntry>("SQLMangaEntry");
    qRegisterMetaType<QByteHash>("QByteHash");
    qRegisterMetaType<QUuid>("QUuid");
    qRegisterMetaType<Z::Ordering>("Z::Ordering");
    qRegisterMetaTypeStreamOperators<ZStrMap>("ZStrMap");

    QApplication a(argc, argv);
    MainWindow w;

#ifdef _WIN32
    QDir appDir(qApp->applicationDirPath());
    qApp->addLibraryPath(appDir.absoluteFilePath("imageformats"));

    if (qApp->arguments().contains("--debug"))
        AllocConsole();
#endif


    w.show();
    
    return a.exec();
}
