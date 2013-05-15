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
#include <magic.h>

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

class ZAbstractReader;
class ZMangaLoader;
class ZMangaView;

namespace Z {

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
    Unordered = 0,
    FileName = 1,
    FileDate = 2,
    AddingDate = 3,
    Album = 4,
    PagesCount = 5
};

}

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
    SQLMangaEntry();
    SQLMangaEntry(int aDbid);
    SQLMangaEntry(const QString& aName, const QString& aFilename, const QString& aAlbum,
                  const QImage &aCover, const int aPagesCount, const qint64 aFileSize,
                  const QString& aFileMagic, const QDateTime& aFileDT, const QDateTime& aAddingDT, int aDbid);
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


extern ZAbstractReader *readerFactory(QObject* parent, QString filename, bool *mimeOk, bool onlyArchives);

QString formatSize(qint64 size);
QString escapeParam(QString param);
int compareWithNumerics(QString ref1, QString ref2);

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

QString detectMIME(QString filename);
QString detectMIME(QByteArray buf);
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
