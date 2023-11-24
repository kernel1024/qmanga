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
#include <QScopedPointer>

#include "imagescale/scalefilter.h"
#include "ocr/abstractocr.h"

#define QSL QStringLiteral // NOLINT
#define QBAL QByteArrayLiteral // NOLINT
#define zF (ZGenericFuncs::instance())

namespace ZDefaults {
const int oneMinuteMS = 60000;
const int oneSecondMS = 1000;
constexpr int oneMegabyte = 1024*1024;
const int resizeTimerInitialMS = 500;
const int resizeTimerDiffMS = 200;
constexpr int maxImageFileSize = 150*oneMegabyte;
constexpr int maxCoverCacheSize = 128*oneMegabyte;
const int albumListWidth = 90;
const int previewWidthMargin = 25;
const int maxDescriptionStringLength = 80;
const int avgSizeSamplesCount = 10;
const int exportFilenameNumWidth = 10;
const int ocrSquareMinimumSize = 20;
const int errorPageLoadMsgVerticalMargin = 5;
const int fileScannerStaticPercentage = 25;
const int fileScannerStaticPercentage2 = 75;
const int maxSQLUpdateStringListSize = 64;
const int magnifySize = 150;
const int scrollDelta = 120;
const int scrollFactor = 5;
const int cacheWidth = 6;
const int magicBlockSize = 1024;
const int textDocMargin = 20;
constexpr double previewProps = 364.0/257.0; // B4 paper proportions
const double oneSecondMSf = 1000.0;
const double dynamicZoomUpScale = 3.0;
const double searchScrollFactor = 0.1;
const double resizeBlur = 1.0;
const double forceDPI = -1.0;
const double standardDPI = 75.0;
constexpr QSize maxDictTooltipSize = QSize(350,350);
constexpr QSize minPreviewSize = QSize(16,qRound(16*ZDefaults::previewProps));
constexpr QSize maxPreviewSize = QSize(512,qRound(512*ZDefaults::previewProps));
constexpr QSize previewSize = QSize(128,qRound(128*ZDefaults::previewProps));
constexpr qint64 copyBlockSize = 2L*oneMegabyte;
const QByteArray coverBase64Header = QBAL("B64#");
}

class ZAbstractReader;
class ZMangaLoader;
class ZMangaView;
class ZPdfController;
class ZDjVuController;
class ZGlobal;

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

enum ReaderFactoryFiltering {
    rffAll = 0,
    rffSkipSingleImageReader = 1
};
Q_ENUM_NS(ReaderFactoryFiltering)

enum ReaderFactoryMode {
    rfmMIMECheckOnly = 0,
    rfmCreateReader = 1
};
Q_ENUM_NS(ReaderFactoryMode)

enum OCREngine {
    ocrTesseract = 0,
    ocrGoogleVision = 1
};
Q_ENUM_NS(OCREngine)

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
    ~ZGenericFuncs() override;
    static ZGenericFuncs* instance();
    static void stdConsoleOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);

    void initialize();
    ZDjVuController* djvuController() const;
    ZGlobal* global() const;
    ZPdfController* pdfController() const;

    static const QHash<Z::Ordering, QString> &getHeaderColumns();
    static const QHash<Z::Ordering, QString> &getSqlColumns();
    static const QHash<Z::Ordering, QString> &getSortMenu();

    static ZAbstractReader *readerFactory(QObject* parent, const QString &filename, bool *mimeOk,
                                          Z::ReaderFactoryFiltering filter, Z::ReaderFactoryMode mode);
    static QStringList supportedImg();
    static QString formatSize(qint64 size);
    static QString elideString(const QString& text, int maxlen, Qt::TextElideMode mode = Qt::ElideRight);
    static QString escapeParam(const QString &param);
    static int compareWithNumerics(const QString &ref1, const QString &ref2);
    static QFileInfoList filterSupportedImgFiles(const QFileInfoList &entryList);
    static QString getOpenFileNameD ( QWidget * parent = nullptr,
                                      const QString & caption = QString(),
                                      const QString & dir = QString(),
                                      const QString & filter = QString(),
                                      QString * selectedFilter = nullptr,
                                      QFileDialog::Options options = QFileDialog::DontUseNativeDialog );
    static QStringList getOpenFileNamesD ( QWidget * parent = nullptr,
                                           const QString & caption = QString(),
                                           const QString & dir = QString(),
                                           const QString & filter = QString(),
                                           QString * selectedFilter = nullptr,
                                           QFileDialog::Options options = QFileDialog::DontUseNativeDialog );
    static QString	getExistingDirectoryD ( QWidget * parent = nullptr,
                                            const QString & caption = QString(),
                                            const QString & dir = QString(),
                                            QFileDialog::Options options = QFileDialog::ShowDirsOnly |
                                                                           QFileDialog::DontUseNativeDialog);
    static QString detectMIME(const QString &filename);
    static QString detectMIME(const QByteArray &buf);
    static QString detectEncodingName(const QByteArray &content);
    static QString detectDecodeToUnicode(const QByteArray &content);
    static QImage resizeImage(const QImage &src, const QSize &targetSize, bool forceFilter = false,
                              Blitz::ScaleFilterType filter = Blitz::LanczosFilter, int page = -1,
                              const int *currentPage = nullptr);
    static void showInGraphicalShell(const QString &pathIn);

    static QString getApplicationDirPath();
    static QByteArray signSHA256withRSA(const QByteArray &data, const QByteArray &privateKey);

    ZAbstractOCR *ocr(QObject *parent = nullptr);

private:
    Q_DISABLE_COPY(ZGenericFuncs)

    QScopedPointer<ZGlobal> m_zg; // keep first!
    QScopedPointer<ZPdfController> m_pdfController;
    QScopedPointer<ZDjVuController> m_djvuController;

};

#endif // GLOBAL_H
