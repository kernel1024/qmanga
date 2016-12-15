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
#include <baseapi.h>
#include <leptonica/allheaders.h>
#endif

#ifndef ARGUNUSED
#define ARGUNUSED(x) (void)(x)
#endif

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

enum PDFRendering {
    Autodetect = 0,
    PageRenderer = 1,
    ImageCatalog = 2
};

enum ResizeFilter {
    Nearest = 0,
    Bilinear = 1,
    Lanczos = 2,
    Gaussian = 3,
    Lanczos2 = 4,
    Cubic = 5,
    Sinc = 6,
    Triangle = 7,
    Mitchell = 8
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

Q_DECLARE_METATYPE(Z::ResizeFilter)
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
    SQLMangaEntry &operator=(const SQLMangaEntry& other);
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
    ZLoaderHelper(QUuid aThreadID);
    ZLoaderHelper(QThread* aThread, ZMangaLoader* aLoader);
    ZLoaderHelper(QPointer<QThread> aThread, QPointer<ZMangaLoader> aLoader);
    ZLoaderHelper &operator=(const ZLoaderHelper& other);
    bool operator==(const ZLoaderHelper& ref) const;
    bool operator!=(const ZLoaderHelper& ref) const;
    void addJob();
    void delJob();
};

class QPageTimer : public QTimer {
    Q_OBJECT
public:
    int savedPage;
    QPageTimer(QObject * parent = 0, int interval = 1000, int pageNum = -1);
};

typedef QList<SQLMangaEntry> SQLMangaList;


extern ZAbstractReader *readerFactory(QObject* parent, QString filename, bool *mimeOk,
                                      bool onlyArchives, bool createReader = true);

QStringList supportedImg();
QString formatSize(qint64 size);
QString escapeParam(QString param);
int compareWithNumerics(QString ref1, QString ref2);
void filterSupportedImgFiles(QFileInfoList& entryList);
void stdConsoleOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);

QString getOpenFileNameD ( QWidget * parent = 0,
                           const QString & caption = QString(),
                           const QString & dir = QString(),
                           const QString & filter = QString(),
                           QString * selectedFilter = 0,
                           QFileDialog::Options options = QFileDialog::DontUseNativeDialog );
QStringList getOpenFileNamesD ( QWidget * parent = 0,
                                const QString & caption = QString(),
                                const QString & dir = QString(),
                                const QString & filter = QString(),
                                QString * selectedFilter = 0,
                                QFileDialog::Options options = QFileDialog::DontUseNativeDialog );
QString getSaveFileNameD ( QWidget * parent = 0,
                           const QString & caption = QString(),
                           const QString & dir = QString(),
                           const QString & filter = QString(),
                           QString * selectedFilter = 0,
                           QFileDialog::Options options = QFileDialog::DontUseNativeDialog );
QString	getExistingDirectoryD ( QWidget * parent = 0,
                                const QString & caption = QString(),
                                const QString & dir = QString(),
                                QFileDialog::Options options = QFileDialog::ShowDirsOnly |
        QFileDialog::DontUseNativeDialog);

QString detectMIME(const QString &filename);
QString detectMIME(const QByteArray &buf);
QPixmap resizeImage(QPixmap src, QSize targetSize,
                    bool forceFilter = false,
                    Z::ResizeFilter filter = Z::Lanczos,
                    ZMangaView* mangaView = NULL);

#ifdef WITH_OCR
extern tesseract::TessBaseAPI* ocr;

tesseract::TessBaseAPI *initializeOCR();
PIX* Image2PIX(QImage& qImage);
QImage PIX2QImage(PIX *pixImage);
#endif

#endif // GLOBAL_H
