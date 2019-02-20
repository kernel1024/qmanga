#include <QApplication>
#include <QDir>
#include <clocale>
#include "mainwindow.h"
#include "global.h"
#include "zdjvureader.h"
#ifdef Q_OS_WIN
#include <windows.h>
#endif

#ifdef WITH_OCR
tesseract::TessBaseAPI* ocr = nullptr;
#endif

int main(int argc, char *argv[])
{
#ifdef JTESS_API4
    setlocale (LC_ALL, "C");
    setlocale (LC_CTYPE, "C");
#endif
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
    qRegisterMetaType<ZExportWork>("ZExportWork");
    qRegisterMetaTypeStreamOperators<ZStrMap>("ZStrMap");
#ifdef WITH_DJVU
    qRegisterMetaType<ZDjVuDocument>("ZDjVuDocument");
#endif


    QApplication a(argc, argv);

    MainWindow w;

#ifdef Q_OS_WIN
    QDir appDir(qApp->applicationDirPath());
    qApp->addLibraryPath(appDir.absoluteFilePath("imageformats"));

    if (qApp->arguments().contains("--debug"))
        AllocConsole();
#endif

    w.show();
    
    return a.exec();
}
