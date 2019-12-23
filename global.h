#ifndef GLOBAL_H
#define GLOBAL_H

#include <QWidget>
#include <QHash>
#include <QPixmap>
#include <QImage>
#include <QFileDialog>
#include <QDateTime>
#include <QUuid>
#include <QPointer>
#include <QTimer>
#include <QThread>

#ifdef WITH_OCR
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

#if TESSERACT_MAJOR_VERSION>=4
    #define JTESS_API4 1
#endif
#endif // WITH_OCR

#include <scalefilter.h>

#define QSL QStringLiteral

namespace ZDefaults {
const int oneMinuteMS = 60000;
const int oneSecondMS = 1000;
const int resizeTimerInitialMS = 500;
const int resizeTimerDiffMS = 200;
const int maxImageFileSize = 150*1024*1024;
const int minPreviewSize = 16;
const int maxPreviewSize = 500;
const int previewSize = 128;
const int albumListWidth = 90;
const int previewWidthMargin = 25;
const int maxDescriptionStringLength = 80;
const int avgSizeSamplesCount = 10;
const int exportFilenameNumWidth = 10;
const int ocrSquareMinimumSize = 20;
const int errorPageLoadMsgVerticalMargin = 5;
const int fileScannerStaticPercentage = 25;
const int maxSQLUpdateStringListSize = 64;
const int magnifySize = 150;
const int scrollDelta = 120;
const int scrollFactor = 5;
const int cacheWidth = 6;
const double previewProps = 364.0/257.0; // B4 paper proportions
const double oneSecondMSf = 1000.0;
const double angle_90deg = 90.0;
const double dynamicZoomUpScale = 3.0;
const double searchScrollFactor = 0.1;
const double resizeBlur = 1.0;
const double forceDPI = -1.0;
const QSize maxDictTooltipSize = QSize(350,350);
const qint64 copyBlockSize = 2*1024*1024L;
}


using QImageHash = QHash<int,QImage>;
using QByteHash = QHash<int,QByteArray>;
using QIntVector = QVector<int>;
using QStrHash = QHash<QString,QString>;

class ZAbstractReader;
class ZMangaLoader;
class ZMangaView;

namespace Z {

// TODO: add prefixes, and Q_ENUM_NS

enum DBMS {
    UndefinedDB = -1,
    MySQL = 0,
    SQLite = 1
};

enum PDFRendering {
    Autodetect = 0,
    PageRenderer = 1,
    ImageCatalog = 2
};

enum Ordering {
    UndefinedOrder = -1,
    Name = 0,
    Album = 1,
    PagesCount = 2,
    FileSize = 3,
    AddingDate = 4,
    CreationFileDate = 5,
    Magic = 6
};

enum PDFImageFormat {
    imgUndefined = -1,
    imgFlate = 1,
    imgDCT = 2
};

static const int maxOrdering = 7;

static const QHash<Ordering,QString> headerColumns = {
    {Name, "Name"},
    {Album, "Album"},
    {PagesCount, "Pages"},
    {FileSize, "Size"},
    {AddingDate, "Added"},
    {CreationFileDate, "Created"},
    {Magic, "Type"}
};

static const QHash<Ordering,QString> sqlColumns = {
    {Name, "files.name"},
    {Album, "albums.name"},
    {PagesCount, "pagesCount"},
    {FileSize, "fileSize"},
    {AddingDate, "addingDT"},
    {CreationFileDate, "fileDT"},
    {Magic,"fileMagic"}
};

static const QHash<Ordering,QString> sortMenu = {
    {Name, "By name"},
    {Album, "By album"},
    {PagesCount, "By pages count"},
    {FileSize, "By file size"},
    {AddingDate, "By adding date"},
    {CreationFileDate, "By file creation date"},
    {Magic, "By file type"}
};

}

Q_DECLARE_METATYPE(Z::Ordering)

class SQLMangaEntry {
public:
    Z::PDFRendering rendering { Z::PDFRendering::Autodetect };
    int dbid { -1 };
    int pagesCount { -1 };
    qint64 fileSize { -1 };
    QString name;
    QString filename;
    QString album;
    QImage cover;
    QString fileMagic;
    QDateTime fileDT;
    QDateTime addingDT;
    SQLMangaEntry() = default;
    ~SQLMangaEntry() = default;
    SQLMangaEntry(const SQLMangaEntry& other);
    SQLMangaEntry(int aDbid);
    SQLMangaEntry(const QString& aName, const QString& aFilename, const QString& aAlbum,
                  const QImage &aCover, const int aPagesCount, const qint64 aFileSize,
                  const QString& aFileMagic, const QDateTime& aFileDT, const QDateTime& aAddingDT,
                  int aDbid, Z::PDFRendering aRendering);
    SQLMangaEntry &operator=(const SQLMangaEntry& other) = default;
    bool operator==(const SQLMangaEntry& ref) const;
    bool operator!=(const SQLMangaEntry& ref) const;
};

class AlbumEntry{
public:
    int id { -1 };
    int parent { -1 };
    QString name;
    AlbumEntry() = default;
    ~AlbumEntry() = default;
    AlbumEntry(int aId) : id(aId), parent(-1) {}
    AlbumEntry(const AlbumEntry& other);
    AlbumEntry(int aId, int aParent, const QString& aName);
    AlbumEntry &operator=(const AlbumEntry& other) = default;
    bool operator==(const AlbumEntry& ref) const;
    bool operator!=(const AlbumEntry& ref) const;
};

class ZLoaderHelper {
public:
    int jobCount { 0 };
    QUuid id;
    QPointer<QThread> thread;
    QPointer<ZMangaLoader> loader;

    ZLoaderHelper();
    ~ZLoaderHelper() = default;
    ZLoaderHelper(const ZLoaderHelper& other);
    ZLoaderHelper(const QUuid& aThreadID);
    ZLoaderHelper(QThread* aThread, ZMangaLoader* aLoader);
    ZLoaderHelper(const QPointer<QThread> &aThread, const QPointer<ZMangaLoader> &aLoader);
    ZLoaderHelper &operator=(const ZLoaderHelper& other) = default;
    bool operator==(const ZLoaderHelper& ref) const;
    bool operator!=(const ZLoaderHelper& ref) const;
    void addJob();
    void delJob();
};

class ZExportWork {
public:
    int filenameLength { 0 };
    int quality { 0 };
    int idx { -1 };
    QDir dir;
    QString format;
    QString sourceFile;
    ZExportWork() = default;
    ~ZExportWork() = default;
    ZExportWork(const ZExportWork& other);
    ZExportWork(int aIdx, const QDir& aDir, const QString& aSourceFile, const QString& aFormat,
                int aFilenameLength, int aQuality);
    ZExportWork &operator=(const ZExportWork& other) = default;
    bool isValid() const;
};

using SQLMangaVector = QVector<SQLMangaEntry>;
using AlbumVector = QVector<AlbumEntry>;


extern ZAbstractReader *readerFactory(QObject* parent, const QString &filename, bool *mimeOk,
                                      bool onlyArchives, bool createReader = true);

QStringList supportedImg();
QString formatSize(qint64 size);
QString elideString(const QString& text, int maxlen, Qt::TextElideMode mode = Qt::ElideRight);
QString escapeParam(const QString &param);
int compareWithNumerics(const QString &ref1, const QString &ref2);
QFileInfoList filterSupportedImgFiles(const QFileInfoList &entryList);
void stdConsoleOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);

QString getOpenFileNameD ( QWidget * parent = nullptr,
                           const QString & caption = QString(),
                           const QString & dir = QString(),
                           const QString & filter = QString(),
                           QString * selectedFilter = nullptr,
                           QFileDialog::Options options = QFileDialog::DontUseNativeDialog );
QStringList getOpenFileNamesD ( QWidget * parent = nullptr,
                                const QString & caption = QString(),
                                const QString & dir = QString(),
                                const QString & filter = QString(),
                                QString * selectedFilter = nullptr,
                                QFileDialog::Options options = QFileDialog::DontUseNativeDialog );
QString	getExistingDirectoryD ( QWidget * parent = nullptr,
                                const QString & caption = QString(),
                                const QString & dir = QString(),
                                QFileDialog::Options options = QFileDialog::ShowDirsOnly |
                                                               QFileDialog::DontUseNativeDialog);

QString detectMIME(const QString &filename);
QString detectMIME(const QByteArray &buf);
QImage resizeImage(const QImage &src, const QSize &targetSize, bool forceFilter = false,
                   Blitz::ScaleFilterType filter = Blitz::LanczosFilter, int page = -1,
                   const int *currentPage = nullptr);

#ifdef WITH_OCR
extern tesseract::TessBaseAPI* ocr;

#ifdef Q_OS_WIN
QString getApplicationDirPath();
#endif

QString ocrGetActiveLanguage();
QString ocrGetDatapath();
tesseract::TessBaseAPI *initializeOCR();
PIX* Image2PIX(const QImage& qImage);
#endif

#endif // GLOBAL_H
