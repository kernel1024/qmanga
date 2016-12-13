#include <QMessageBox>
#include <QDebug>

#include "zmangaloader.h"
#include "global.h"
#include "zabstractreader.h"
#include "zzipreader.h"
#include "zrarreader.h"
#include "zpdfreader.h"
#include "zimagesdirreader.h"
#include "zsingleimagereader.h"
#include "zglobal.h"
#include "zmangaview.h"

#ifdef WITH_MAGICK
#define MAGICKCORE_QUANTUM_DEPTH 8
#define MAGICKCORE_HDRI_ENABLE 0

#include <Magick++.h>

using namespace Magick;
#endif

ZAbstractReader *readerFactory(QObject* parent, QString filename, bool *mimeOk,
                               bool onlyArchives, bool createReader)
{
    *mimeOk = true;
    if (filename.startsWith(":CLIP:") && !onlyArchives) {
        if (createReader)
            return new ZSingleImageReader(parent,filename);
        else
            return NULL;
    }

    if (filename.startsWith("#DYN#")) {
        QString fname(filename);
        fname.remove(QRegExp("^#DYN#"));
        if (createReader)
            return new ZImagesDirReader(parent,fname);
        else
            return NULL;

    }
    QFileInfo fi(filename);
    if (!fi.exists()) return NULL;

    QString mime = detectMIME(filename).toLower();
    if (mime.contains("application/zip",Qt::CaseInsensitive)) {
        if (createReader)
            return new ZZipReader(parent,filename);
        else
            return NULL;
    } else if (mime.contains("application/x-rar",Qt::CaseInsensitive) ||
               mime.contains("application/rar",Qt::CaseInsensitive)) {
        if (createReader)
            return new ZRarReader(parent,filename);
        else
            return NULL;
#ifdef WITH_POPPLER
    } else if (mime.contains("application/pdf",Qt::CaseInsensitive)) {
        if (createReader)
            return new ZPdfReader(parent,filename);
        else
            return NULL;
#endif
    } else if (mime.contains("image/",Qt::CaseInsensitive) && !onlyArchives) {
        if (createReader)
            return new ZSingleImageReader(parent,filename);
        else
            return NULL;
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
    QStringList r1nums,r1sep;
    QString sn,sc;
    for (int i=0;i<=ref1.length();i++) {
        if (!sn.isEmpty() &&
            ((i==ref1.length()) ||
             (!ref1[i].isDigit() && sn[sn.length()-1].isDigit()))) {
            r1nums << sn;
            r1sep << sc;
            sn.clear();
            sc.clear();
        }
        if (i<ref1.length()) {
            if (ref1[i].isDigit())
                sn.append(ref1[i]);
            else if (ref1[i].isLetter())
                sc.append(ref1[i]);
        } else {
            sc.clear();
            sn.clear();
        }
    }
    QStringList r2nums,r2sep;
    sn.clear();
    sc.clear();
    for (int i=0;i<=ref2.length();i++) {
        if (!sn.isEmpty() &&
            ((i==ref2.length()) ||
             (!ref2[i].isDigit() && sn[sn.length()-1].isDigit()))) {
            r2nums << sn;
            r2sep << sc;
            sn.clear();
            sc.clear();
        }
        if (i<ref2.length()) {
            if (ref2[i].isDigit())
                sn.append(ref2[i]);
            else if (ref2[i].isLetter())
                sc.append(ref2[i]);
        } else {
            sc.clear();
            sn.clear();
        }
    }
//    QStringList r1nums = ref1.split(QRegExp("\\D+"),QString::SkipEmptyParts);
//    QStringList r2nums = ref2.split(QRegExp("\\D+"),QString::SkipEmptyParts);
    int mlen = qMin(r1nums.count(),r2nums.count());
    for (int i=0;i<mlen;i++) {
        bool okconv;
        int r1n = r1nums.at(i).toInt(&okconv);
        if (!okconv) break;
        int r2n = r2nums.at(i).toInt(&okconv);
        if (!okconv) break;

        if (r1sep.at(i)<r2sep.at(i)) return -1;
        else if (r1sep.at(i)>r2sep.at(i)) return 1;
        else if (r1n<r2n) return -1;
        else if (r1n>r2n) return 1;
    }
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
    rendering = Z::PDFRendering::Autodetect;
}

SQLMangaEntry::SQLMangaEntry(const SQLMangaEntry &other)
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
    rendering = other.rendering;
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
    rendering = Z::PDFRendering::Autodetect;
}

SQLMangaEntry::SQLMangaEntry(const QString &aName, const QString &aFilename, const QString &aAlbum,
                             const QImage &aCover, const int aPagesCount, const qint64 aFileSize,
                             const QString &aFileMagic, const QDateTime &aFileDT,
                             const QDateTime &aAddingDT, int aDbid, Z::PDFRendering aRendering)
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
    rendering = aRendering;
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
    rendering = other.rendering;
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

ZLoaderHelper::ZLoaderHelper(const ZLoaderHelper &other)
{
    id = other.id;
    thread = other.thread;
    loader = other.loader;
    jobCount = other.jobCount;
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


QStringList supportedImg()
{
    QStringList res {"jpg", "jpeg", "jpe", "gif", "png", "tiff", "tif", "bmp"};
    return res;
}

void filterSupportedImgFiles(QFileInfoList& entryList)
{
    int idx = 0;
    QStringList suffices = supportedImg();
    while (idx<entryList.count()) {
        if (!suffices.contains(entryList.at(idx).suffix(),Qt::CaseInsensitive))
            entryList.removeAt(idx);
        else
            idx++;
    }
}
