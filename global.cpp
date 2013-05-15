#include <QMessageBox>

#include "zmangaloader.h"
#include "global.h"
#include "zabstractreader.h"
#include "zzipreader.h"
#include "zrarreader.h"
#include "zpdfreader.h"
#include "zsingleimagereader.h"
#include "zglobal.h"
#include "zmangaview.h"

#ifdef WITH_MAGICK
#define MAGICKCORE_QUANTUM_DEPTH 8
#define MAGICKCORE_HDRI_ENABLE 0

#include <Magick++.h>

using namespace Magick;
#endif

ZAbstractReader *readerFactory(QObject* parent, QString filename, bool *mimeOk, bool onlyArchives)
{
    *mimeOk = true;
    if (filename.startsWith(":CLIP:") && !onlyArchives) {
        return new ZSingleImageReader(parent,filename);
    }
    QFileInfo fi(filename);
    if (!fi.exists()) return NULL;

    QString mime = detectMIME(filename).toLower();
    if (mime.contains("application/zip",Qt::CaseInsensitive)) {
        return new ZZipReader(parent,filename);
    } else if (mime.contains("application/x-rar",Qt::CaseInsensitive) ||
               mime.contains("application/rar",Qt::CaseInsensitive)) {
        return new ZRarReader(parent,filename);
#ifdef WITH_POPPLER
    } else if (mime.contains("application/pdf",Qt::CaseInsensitive)) {
        return new ZPdfReader(parent,filename);
#endif
    } else if (mime.contains("image/",Qt::CaseInsensitive) && !onlyArchives) {
        return new ZSingleImageReader(parent,filename);
    } else {
        *mimeOk = false;
        return NULL;
    }
}

QString formatSize(qint64 size)
{
    if (size<1024) return QString("%1 bytes").arg(size);
    else if (size<1024*1024) return QString("%1 Kb").arg(size/1024);
    else if (size<1024*1024*1024) return QString("%1 Mb").arg(size/(1024*1024));
    else return QString("%1 Gb").arg(size/(1024*1024*1024));
}

QString escapeParam(QString param)
{
    QString res = param;
    res.replace("'","\\'");
    res.replace("\"","\\\"");
    res.replace(";","");
    res.replace("%","");
    return res;
}

int compareWithNumerics(QString ref1, QString ref2)
{
    // find common part of both strings
    int mlen = qMin(ref1.length(),ref2.length());
    int diffs = 0;
    for (int i=0;i<mlen;i++) {
        if (ref1.at(i).isDigit() || ref2.at(i).isDigit()) {
            diffs = i;
            break;
        }
        if (ref1.at(i)!=ref2.at(i)) {
            diffs = i;
            break;
        }
    }
    if (diffs>0) // and compare only differences as strings again
        return compareWithNumerics(ref1.mid(diffs),ref2.mid(diffs));

    // try convert to int
    bool okconv1, okconv2;
    int v1 = ref1.toInt(&okconv1);
    int v2 = ref2.toInt(&okconv2);
    if (okconv1 && okconv2) {
        if (v1<v2) return -1;
        else if (v1==v2) return 0;
        else return 1;
    }

    // simply compare strings
    return QString::compare(ref1,ref2);
}

QString getOpenFileNameD ( QWidget * parent, const QString & caption, const QString & dir, const QString & filter, QString * selectedFilter, QFileDialog::Options options )
{
    return QFileDialog::getOpenFileName(parent,caption,dir,filter,selectedFilter,options);
}

QStringList getOpenFileNamesD ( QWidget * parent, const QString & caption, const QString & dir, const QString & filter, QString * selectedFilter, QFileDialog::Options options )
{
    return QFileDialog::getOpenFileNames(parent,caption,dir,filter,selectedFilter,options);
}

QString getSaveFileNameD ( QWidget * parent, const QString & caption, const QString & dir, const QString & filter, QString * selectedFilter, QFileDialog::Options options )
{
    return QFileDialog::getSaveFileName(parent,caption,dir,filter,selectedFilter,options);
}

QString	getExistingDirectoryD ( QWidget * parent, const QString & caption, const QString & dir, QFileDialog::Options options )
{
    return QFileDialog::getExistingDirectory(parent,caption,dir,options);
}

QString detectMIME(QString filename)
{
    magic_t myt = magic_open(MAGIC_ERROR|MAGIC_MIME_TYPE);
    magic_load(myt,NULL);
    QByteArray bma = filename.toUtf8();
    const char* bm = bma.data();
    const char* mg = magic_file(myt,bm);
    if (mg==NULL) {
        qDebug() << "libmagic error: " << magic_errno(myt) << QString::fromUtf8(magic_error(myt));
        return QString("text/plain");
    }
    QString mag(mg);
    magic_close(myt);
    return mag;
}

QString detectMIME(QByteArray buf)
{
    magic_t myt = magic_open(MAGIC_ERROR|MAGIC_MIME_TYPE);
    magic_load(myt,NULL);
    const char* mg = magic_buffer(myt,buf.data(),buf.length());
    if (mg==NULL) {
        qDebug() << "libmagic error: " << magic_errno(myt) << QString::fromUtf8(magic_error(myt));
        return QString("text/plain");
    }
    QString mag(mg);
    magic_close(myt);
    return mag;
}

QPixmap resizeImage(QPixmap src, QSize targetSize, bool forceFilter, Z::ResizeFilter filter,
                    ZMangaView *mangaView)
{
    static bool imagickOk = true;
    Z::ResizeFilter rf = zg->resizeFilter;
    if (forceFilter)
        rf = filter;
    if (rf==Z::Nearest || !imagickOk)
        return src.scaled(targetSize,Qt::IgnoreAspectRatio,Qt::FastTransformation);
    else if (rf==Z::Bilinear)
        return src.scaled(targetSize,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
#ifndef WITH_MAGICK
    else
        return src.scaled(targetSize,Qt::IgnoreAspectRatio,Qt::FastTransformation);
#else
    else {
        QTime tmr;
        tmr.start();

        QPixmap dst = QPixmap();
        try {
            // Load QImage to ImageMagick image
            QByteArray bufSrc;
            QBuffer buf(&bufSrc);
            buf.open(QIODevice::WriteOnly);
            src.save(&buf,"BMP");
            buf.close();
            Blob iBlob(bufSrc.data(),bufSrc.size());
            Image iImage(iBlob,Geometry(src.width(),src.height()),"BMP");

            // Resize image
            if (rf==Z::Lanczos)
                iImage.filterType(LanczosFilter);
            else if (rf==Z::Gaussian)
                iImage.filterType(GaussianFilter);
            else if (rf==Z::Lanczos2)
                iImage.filterType(Lanczos2Filter);
            else if (rf==Z::Cubic)
                iImage.filterType(CubicFilter);
            else if (rf==Z::Sinc)
                iImage.filterType(SincFilter);
            else if (rf==Z::Triangle)
                iImage.filterType(TriangleFilter);
            else if (rf==Z::Mitchell)
                iImage.filterType(MitchellFilter);
            else
                iImage.filterType(LanczosFilter);

            iImage.resize(Geometry(targetSize.width(),targetSize.height()));

            // Convert image to QImage
            Blob oBlob;
            iImage.magick("BMP");
            iImage.write(&oBlob);
            bufSrc.clear();
            bufSrc=QByteArray::fromRawData((char*)(oBlob.data()),oBlob.length());
            dst.loadFromData(bufSrc);
            bufSrc.clear();
        } catch (...) {
            imagickOk = false;
            qDebug() << "ImageMagick crashed. Using Qt image scaling.";
            if (mangaView!=NULL)
                QMetaObject::invokeMethod(mangaView,"asyncMsg",Qt::QueuedConnection,
                                          Q_ARG(QString,"ImageMagick crashed. Using Qt image scaling."));
            return src.scaled(targetSize,Qt::IgnoreAspectRatio,Qt::FastTransformation);
        }

        //qDebug() << "ImageMagick ms: " << tmr.elapsed();
        return dst;
    }
#endif
}

SQLMangaEntry::SQLMangaEntry()
{
    name = QString();
    filename = QString();
    cover = QImage();
    album = QString();
    pagesCount = -1;
    fileSize = -1;
    fileMagic = QString();
    fileDT = QDateTime();
    addingDT = QDateTime();
    dbid = -1;
}

SQLMangaEntry::SQLMangaEntry(int aDbid)
{
    name = QString();
    filename = QString();
    cover = QImage();
    album = QString();
    pagesCount = -1;
    fileSize = -1;
    fileMagic = QString();
    fileDT = QDateTime();
    addingDT = QDateTime();
    dbid = aDbid;
}

SQLMangaEntry::SQLMangaEntry(const QString &aName, const QString &aFilename, const QString &aAlbum,
                             const QImage &aCover, const int aPagesCount, const qint64 aFileSize,
                             const QString &aFileMagic, const QDateTime &aFileDT,
                             const QDateTime &aAddingDT, int aDbid)
{
    name = aName;
    filename = aFilename;
    album = aAlbum;
    cover = aCover;
    pagesCount = aPagesCount;
    fileSize = aFileSize;
    fileMagic = aFileMagic;
    fileDT = aFileDT;
    addingDT = aAddingDT;
    dbid = aDbid;
}

SQLMangaEntry &SQLMangaEntry::operator =(const SQLMangaEntry &other)
{
    name = other.name;
    filename = other.filename;
    album = other.album;
    cover = other.cover;
    pagesCount = other.pagesCount;
    fileSize = other.fileSize;
    fileMagic = other.fileMagic;
    fileDT = other.fileDT;
    addingDT = other.addingDT;
    dbid = other.dbid;
    return *this;
}

bool SQLMangaEntry::operator ==(const SQLMangaEntry &ref) const
{
    return (ref.dbid==dbid);
}

bool SQLMangaEntry::operator !=(const SQLMangaEntry &ref) const
{
    return (ref.dbid!=dbid);
}

ZLoaderHelper::ZLoaderHelper()
{
    id = QUuid::createUuid();
    jobCount = 0;
}

ZLoaderHelper::ZLoaderHelper(QUuid aThreadID)
{
    id = aThreadID;
    jobCount = 0;
}

ZLoaderHelper::ZLoaderHelper(QThread *aThread, ZMangaLoader *aLoader)
{
    id = QUuid::createUuid();
    thread = aThread;
    loader = aLoader;
    jobCount = 0;
}

ZLoaderHelper::ZLoaderHelper(QPointer<QThread> aThread, QPointer<ZMangaLoader> aLoader)
{
    id = QUuid::createUuid();
    thread = aThread;
    loader = aLoader;
    jobCount = 0;
}

ZLoaderHelper &ZLoaderHelper::operator =(const ZLoaderHelper &other)
{
    id = other.id;
    thread = other.thread;
    loader = other.loader;
    jobCount = other.jobCount;
    return *this;
}

bool ZLoaderHelper::operator ==(const ZLoaderHelper &ref) const
{
    return (id == ref.id);
}

bool ZLoaderHelper::operator !=(const ZLoaderHelper &ref) const
{
    return (id != ref.id);
}

void ZLoaderHelper::addJob()
{
    jobCount++;
}

void ZLoaderHelper::delJob()
{
    if (jobCount>0)
        jobCount--;
}


QPageTimer::QPageTimer(QObject *parent, int interval, int pageNum)
    : QTimer(parent)
{
    setSingleShot(true);
    setInterval(interval);
    savedPage = pageNum;
}

#ifdef WITH_OCR
PIX* Image2PIX(QImage& qImage) {
  PIX * pixs;
  l_uint32 *lines;

  qImage = qImage.rgbSwapped();
  int width = qImage.width();
  int height = qImage.height();
  int depth = qImage.depth();
  int wpl = qImage.bytesPerLine() / 4;

  pixs = pixCreate(width, height, depth);
  pixSetWpl(pixs, wpl);
  pixSetColormap(pixs, NULL);
  l_uint32 *datas = pixs->data;

  for (int y = 0; y < height; y++) {
    lines = datas + y * wpl;
    QByteArray a((const char*)qImage.scanLine(y), qImage.bytesPerLine());
    for (int j = 0; j < a.size(); j++) {
      *((l_uint8 *)lines + j) = a[j];
    }
  }
  return pixEndianByteSwapNew(pixs);
}

QImage PIX2QImage(PIX *pixImage) {
  int width = pixGetWidth(pixImage);
  int height = pixGetHeight(pixImage);
  int depth = pixGetDepth(pixImage);
  int bytesPerLine = pixGetWpl(pixImage) * 4;
  l_uint32 * s_data = pixGetData(pixEndianByteSwapNew(pixImage));

  QImage::Format format;
  if (depth == 1)
    format = QImage::Format_Mono;
  else if (depth == 8)
    format = QImage::Format_Indexed8;
  else
    format = QImage::Format_RGB32;

  QImage result((uchar*)s_data, width, height, bytesPerLine, format);

  // Handle pallete
  QVector<QRgb> _bwCT;
  _bwCT.append(qRgb(255,255,255));
  _bwCT.append(qRgb(0,0,0));

  QVector<QRgb> _grayscaleCT(256);
  for (int i = 0; i < 256; i++)  {
    _grayscaleCT.append(qRgb(i, i, i));
  }
  if (depth == 1) {
    result.setColorTable(_bwCT);
  }  else if (depth == 8)  {
    result.setColorTable(_grayscaleCT);

  } else {
    result.setColorTable(_grayscaleCT);
  }

  if (result.isNull()) {
    static QImage none(0,0,QImage::Format_Invalid);
    qDebug() << "***Invalid format!!!";
    return none;
  }

  return result.rgbSwapped();
}

tesseract::TessBaseAPI* initializeOCR()
{
    ocr = new tesseract::TessBaseAPI();
    if (ocr->Init(NULL,"jpn")) {
        QMessageBox::critical(NULL,"QManga error",
                              "Could not initialize Tesseract.\n"
                              "Maybe japanese language training data is not installed.");
        return NULL;
    }
    return ocr;
}
#endif
