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
#endif // WITH_OCR

#include <scalefilter.h>

#define QSL QStringLiteral
#define zF (ZGenericFuncs::instance())

namespace ZDefaults {
constexpr int oneMinuteMS = 60000;
constexpr int oneSecondMS = 1000;
constexpr int resizeTimerInitialMS = 500;
constexpr int resizeTimerDiffMS = 200;
constexpr int maxImageFileSize = 150*1024*1024;
constexpr int minPreviewSize = 16;
constexpr int maxPreviewSize = 500;
constexpr int previewSize = 128;
constexpr int albumListWidth = 90;
constexpr int previewWidthMargin = 25;
constexpr int maxDescriptionStringLength = 80;
constexpr int avgSizeSamplesCount = 10;
constexpr int exportFilenameNumWidth = 10;
constexpr int ocrSquareMinimumSize = 20;
constexpr int errorPageLoadMsgVerticalMargin = 5;
constexpr int fileScannerStaticPercentage = 25;
constexpr int maxSQLUpdateStringListSize = 64;
constexpr int magnifySize = 150;
constexpr int scrollDelta = 120;
constexpr int scrollFactor = 5;
constexpr int cacheWidth = 6;
constexpr int magicBlockSize = 1024;
constexpr double previewProps = 364.0/257.0; // B4 paper proportions
constexpr double oneSecondMSf = 1000.0;
constexpr double angle_90deg = 90.0;
constexpr double dynamicZoomUpScale = 3.0;
constexpr double searchScrollFactor = 0.1;
constexpr double resizeBlur = 1.0;
constexpr double forceDPI = -1.0;
constexpr double standardDPI = 75.0;
constexpr QSize maxDictTooltipSize = QSize(350,350);
constexpr qint64 copyBlockSize = 2*1024*1024L;
}

class ZAbstractReader;
class ZMangaLoader;
class ZMangaView;

namespace Z {
Q_NAMESPACE

enum DBMS {
    dbmsUndefinedDB = -1,
    dbmsMySQL = 0,
    dbmsSQLite = 1
};
Q_ENUM_NS(DBMS)

enum PDFRendering {
    pdfAutodetect = 0,
    pdfPageRenderer = 1,
    pdfImageCatalog = 2
};
Q_ENUM_NS(PDFRendering)

enum Ordering {
    ordUndefined = -1,
    ordName = 0,
    ordAlbum = 1,
    ordPagesCount = 2,
    ordFileSize = 3,
    ordAddingDate = 4,
    ordCreationFileDate = 5,
    ordMagic = 6
};
Q_ENUM_NS(Ordering)

enum PDFImageFormat {
    imgUndefined = -1,
    imgFlate = 1,
    imgDCT = 2
};
Q_ENUM_NS(PDFImageFormat)

static const int maxOrdering = 7;

static const QHash<Ordering,QString> headerColumns = {
    {ordName, "Name"},
    {ordAlbum, "Album"},
    {ordPagesCount, "Pages"},
    {ordFileSize, "Size"},
    {ordAddingDate, "Added"},
    {ordCreationFileDate, "Created"},
    {ordMagic, "Type"}
};

static const QHash<Ordering,QString> sqlColumns = {
    {ordName, "files.name"},
    {ordAlbum, "albums.name"},
    {ordPagesCount, "pagesCount"},
    {ordFileSize, "fileSize"},
    {ordAddingDate, "addingDT"},
    {ordCreationFileDate, "fileDT"},
    {ordMagic,"fileMagic"}
};

static const QHash<Ordering,QString> sortMenu = {
    {ordName, "By name"},
    {ordAlbum, "By album"},
    {ordPagesCount, "By pages count"},
    {ordFileSize, "By file size"},
    {ordAddingDate, "By adding date"},
    {ordCreationFileDate, "By file creation date"},
    {ordMagic, "By file type"}
};

}

Q_DECLARE_METATYPE(Z::Ordering)

class ZSQLMangaEntry {
public:
    Z::PDFRendering rendering { Z::PDFRendering::pdfAutodetect };
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
    ZSQLMangaEntry() = default;
    ~ZSQLMangaEntry() = default;
    ZSQLMangaEntry(const ZSQLMangaEntry& other);
    explicit ZSQLMangaEntry(int aDbid);
    ZSQLMangaEntry(const QString& aName, const QString& aFilename, const QString& aAlbum,
                  const QImage &aCover, int aPagesCount, qint64 aFileSize,
                  const QString& aFileMagic, const QDateTime& aFileDT, const QDateTime& aAddingDT,
                  int aDbid, Z::PDFRendering aRendering);
    ZSQLMangaEntry &operator=(const ZSQLMangaEntry& other) = default;
    bool operator==(const ZSQLMangaEntry& ref) const;
    bool operator!=(const ZSQLMangaEntry& ref) const;
};

class ZAlbumEntry{
public:
    int id { -1 };
    int parent { -1 };
    QString name;
    ZAlbumEntry() = default;
    ~ZAlbumEntry() = default;
    explicit ZAlbumEntry(int aId) : id(aId) {}
    ZAlbumEntry(const ZAlbumEntry& other);
    ZAlbumEntry(int aId, int aParent, const QString& aName);
    ZAlbumEntry &operator=(const ZAlbumEntry& other) = default;
    bool operator==(const ZAlbumEntry& ref) const;
    bool operator!=(const ZAlbumEntry& ref) const;
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
    explicit ZLoaderHelper(const QUuid& aThreadID);
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

using ZIntVector = QVector<int>;
using ZStrHash = QHash<QString,QString>;
using ZSQLMangaVector = QVector<ZSQLMangaEntry>;
using ZAlbumVector = QVector<ZAlbumEntry>;
using ZStrMap = QMap<QString, QString>;

class ZGenericFuncs : public QObject
{
    Q_OBJECT
public:
    explicit ZGenericFuncs(QObject *parent = nullptr);
    ~ZGenericFuncs() override = default;
    static ZGenericFuncs* instance();
    static void stdConsoleOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);

    void initialize();

    ZAbstractReader *readerFactory(QObject* parent, const QString &filename, bool *mimeOk,
                                          bool onlyArchives, bool createReader = true);
    QStringList supportedImg();
    QString formatSize(qint64 size);
    QString elideString(const QString& text, int maxlen, Qt::TextElideMode mode = Qt::ElideRight);
    QString escapeParam(const QString &param);
    int compareWithNumerics(const QString &ref1, const QString &ref2);
    QFileInfoList filterSupportedImgFiles(const QFileInfoList &entryList);
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
#ifdef Q_OS_WIN
    QString getApplicationDirPath();
#endif
    QString ocrGetActiveLanguage();
    QString ocrGetDatapath();
    void initializeOCR();
    QString processImageWithOCR(const QImage& image);
    bool isOCRReady() const;
#endif

private:
    Q_DISABLE_COPY(ZGenericFuncs)

#ifdef WITH_OCR
    tesseract::TessBaseAPI* m_ocr { nullptr };
    PIX* Image2PIX(const QImage& qImage);
#endif

};

#endif // GLOBAL_H
