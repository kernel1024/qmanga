#ifndef GLOBAL_H
#define GLOBAL_H

#include <QWidget>
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

#define maxPreviewSize 500
#define previewProps 364/257

typedef QHash<int,QImage> QImageHash;
typedef QHash<int,QByteArray> QByteHash;
typedef QList<int> QIntList;
typedef QHash<QString,QString> QStrHash;

class ZAbstractReader;
class ZMangaLoader;
class ZMangaView;

namespace Z {

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
    Name = 0,
    Album = 1,
    PagesCount = 2,
    FileSize = 3,
    AddingDate = 4,
    CreationFileDate = 5,
    Magic = 6
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
    int dbid;
    QString name;
    QString filename;
    QString album;
    QImage cover;
    int pagesCount;
    qint64 fileSize;
    QString fileMagic;
    QDateTime fileDT;
    QDateTime addingDT;
    Z::PDFRendering rendering;
    SQLMangaEntry();
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

class ZLoaderHelper {
public:
    QUuid id;
    QPointer<QThread> thread;
    QPointer<ZMangaLoader> loader;
    int jobCount;

    ZLoaderHelper();
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
    QDir dir;
    QString format;
    QString sourceFile;
    int filenameLength;
    int quality;
    int idx;
    ZExportWork();
    ZExportWork(const ZExportWork& other);
    ZExportWork(int aIdx, const QDir& aDir, const QString& aSourceFile, const QString& aFormat,
                int aFilenameLength, int aQuality);
    ZExportWork &operator=(const ZExportWork& other) = default;
    bool isValid() const;
};

class QPageTimer : public QTimer {
    Q_OBJECT
public:
    int savedPage;
    QPageTimer(QObject * parent = nullptr, int interval = 1000, int pageNum = -1);
};

typedef QList<SQLMangaEntry> SQLMangaList;


extern ZAbstractReader *readerFactory(QObject* parent, const QString &filename, bool *mimeOk,
                                      bool onlyArchives, bool createReader = true);

QStringList supportedImg();
QString formatSize(qint64 size);
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
QString getSaveFileNameD ( QWidget * parent = nullptr,
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
                    Blitz::ScaleFilterType filter = Blitz::LanczosFilter, int page = -1, const int *currentPage = nullptr);

#ifdef WITH_OCR
extern tesseract::TessBaseAPI* ocr;

QString ocrGetActiveLanguage();
QString ocrGetDatapath();
tesseract::TessBaseAPI *initializeOCR();
PIX* Image2PIX(const QImage& qImage);
QImage PIX2QImage(PIX *pixImage);
#endif

#endif // GLOBAL_H
