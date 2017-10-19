#include <QMessageBox>
#include <QMutex>
#include <QTextCodec>
#include <QTextEncoder>
#include <QBuffer>
#include <QDir>
#include <QApplication>
#include <QDebug>

#include "zmangaloader.h"
#include "global.h"
#include "zabstractreader.h"
#include "zzipreader.h"
#include "zrarreader.h"
#include "zpdfreader.h"
#include "zdjvureader.h"
#include "zimagesdirreader.h"
#include "zsingleimagereader.h"
#include "zglobal.h"
#include "zmangaview.h"
#include "scalefilter.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#ifdef WITH_LIBMAGIC
#include <magic.h>

#else
// some magic here!

struct MagicSample {
    qint64 offset;
    qint64 size;
    const char* sample;
    const char* mime;
};

static const MagicSample magicList[] = {
    { 0, 4, "\x50\x4b\x03\x04", "application/zip" },
    { 0, 4, "\x52\x61\x72\x21", "application/rar" },
    { 0, 4, "\x25\x50\x44\x46", "application/pdf" },
    { 0, 4, "\xFF\xD8\xFF\xDB", "image/jpeg" },
    { 6, 4, "\x4A\x46\x49\x46", "image/jpeg" },
    { 6, 4, "\x45\x78\x69\x66", "image/jpeg" },
    { 0, 4, "\x89\x50\x4E\x47", "image/png" },
    { 0, 6, "\x47\x49\x46\x38\x37\x61", "image/gif" },
    { 0, 6, "\x47\x49\x46\x38\x39\x61", "image/gif" },
    { 0, 4, "\x49\x49\x2A\x00", "image/tiff" },
    { 0, 4, "\x4D\x4D\x00\x2A", "image/tiff" },
    { 0, 2, "\x42\x4D", "image/bmp" },
    { 0, 0, nullptr, nullptr }
};

#endif

ZAbstractReader *readerFactory(QObject* parent, QString filename, bool *mimeOk,
                               bool onlyArchives, bool createReader)
{
    *mimeOk = true;
    if (filename.startsWith(":CLIP:") && !onlyArchives) {
        if (createReader)
            return new ZSingleImageReader(parent,filename);
        else
            return nullptr;
    }

    if (filename.startsWith("#DYN#")) {
        QString fname(filename);
        fname.remove(QRegExp("^#DYN#"));
        if (createReader)
            return new ZImagesDirReader(parent,fname);
        else
            return nullptr;

    }
    QFileInfo fi(filename);
    if (!fi.exists()) return nullptr;

    QString mime = detectMIME(filename).toLower();
    if (mime.contains("application/zip",Qt::CaseInsensitive)) {
        if (createReader)
            return new ZZipReader(parent,filename);
        else
            return nullptr;
    } else if (mime.contains("application/x-rar",Qt::CaseInsensitive) ||
               mime.contains("application/rar",Qt::CaseInsensitive)) {
        if (createReader)
            return new ZRarReader(parent,filename);
        else
            return nullptr;
#ifdef WITH_POPPLER
    } else if (mime.contains("application/pdf",Qt::CaseInsensitive)) {
        if (createReader)
            return new ZPdfReader(parent,filename);
        else
            return nullptr;
#endif
#ifdef WITH_DJVU
    } else if (mime.contains("image/vnd.djvu",Qt::CaseInsensitive)) {
        if (createReader)
            return new ZDjVuReader(parent,filename);
        else
            return nullptr;
#endif
    } else if (mime.contains("image/",Qt::CaseInsensitive) && !onlyArchives) {
        if (createReader)
            return new ZSingleImageReader(parent,filename);
        else
            return nullptr;
    } else {
        *mimeOk = false;
        return nullptr;
    }
}

wchar_t *toUtf16(const QString &str)
{
    wchar_t* res = new wchar_t[str.length()];
    str.toWCharArray(res);
    return res;
}

void stdConsoleOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    static QMutex loggerMutex;

    loggerMutex.lock();

    QString lmsg = QString();

    int line = context.line;
    QString file = QString(context.file);
    QString category = QString(context.category);
    if (category==QString("default"))
        category.clear();
    else
        category.append(' ');

    switch (type) {
        case QtDebugMsg:
            lmsg = QString("%1Debug: %2 (%3:%4)").arg(category, msg, file, QString("%1").arg(line));
            break;
        case QtWarningMsg:
            lmsg = QString("%1Warning: %2 (%3:%4)").arg(category, msg, file, QString("%1").arg(line));
            break;
        case QtCriticalMsg:
            lmsg = QString("%1Critical: %2 (%3:%4)").arg(category, msg, file, QString("%1").arg(line));
            break;
        case QtFatalMsg:
            lmsg = QString("%1Fatal: %2 (%3:%4)").arg(category, msg, file, QString("%1").arg(line));
            break;
        case QtInfoMsg:
            lmsg = QString("%1Info: %2 (%3:%4)").arg(category, msg, file, QString("%1").arg(line));
            break;
    }

    if (!lmsg.isEmpty()) {
        QString fmsg = QTime::currentTime().toString("h:mm:ss") + " "+lmsg;
        fmsg.append("\r\n");

#ifdef Q_OS_WIN
        HANDLE con = GetStdHandle(STD_ERROR_HANDLE);
        wchar_t* wmsg = toUtf16(fmsg);
        DWORD wr = 0;
        WriteConsoleW(con,wmsg,fmsg.length(),&wr,NULL);
        delete[] wmsg;
#endif
        fprintf(stderr, "%s", fmsg.toLocal8Bit().constData());
    }

    loggerMutex.unlock();
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

QString detectMIME(const QString& filename)
{
#ifdef WITH_LIBMAGIC
    magic_t myt = magic_open(MAGIC_ERROR|MAGIC_MIME_TYPE);
    magic_load(myt,nullptr);
    QByteArray bma = filename.toUtf8();
    const char* bm = bma.data();
    const char* mg = magic_file(myt,bm);
    if (mg==nullptr) {
        qDebug() << "libmagic error: " << magic_errno(myt) << QString::fromUtf8(magic_error(myt));
        return QString("text/plain");
    }
    QString mag(mg);
    magic_close(myt);
    return mag;
#else
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly))
        return QString("text/plain");
    QByteArray ba = f.read(1024);
    f.close();
    return detectMIME(ba);
#endif
}

QString detectMIME(const QByteArray &buf)
{
#ifdef WITH_LIBMAGIC
    magic_t myt = magic_open(MAGIC_ERROR|MAGIC_MIME_TYPE);
    magic_load(myt,nullptr);
    const char* mg = magic_buffer(myt,buf.data(),buf.length());
    if (mg==nullptr) {
        qDebug() << "libmagic error: " << magic_errno(myt) << QString::fromUtf8(magic_error(myt));
        return QString("text/plain");
    }
    QString mag(mg);
    magic_close(myt);
    return mag;
#else
    for (int idx=0;magicList[idx].sample!=nullptr;idx++) {
        QByteArray test = buf.mid(magicList[idx].offset,magicList[idx].size);
        QByteArray sample = QByteArray::fromRawData(magicList[idx].sample,magicList[idx].size);
        if (test==sample)
            return QString::fromLatin1(magicList[idx].mime);
    }
    return QString("text/plain");
#endif
}

QImage resizeImage(const QImage& src, const QSize& targetSize, bool forceFilter,
                   Blitz::ScaleFilterType filter, int page, const int *currentPage)
{
    QSize dsize = src.size().scaled(targetSize,Qt::KeepAspectRatio);

    Blitz::ScaleFilterType rf = zg->downscaleFilter;
    if (dsize.width()>src.size().width())
        rf = zg->upscaleFilter;

    if (forceFilter)
        rf = filter;

    if (rf==Blitz::UndefinedFilter)
        return src.scaled(targetSize,Qt::IgnoreAspectRatio,Qt::FastTransformation);
    else if (rf==Blitz::Bilinear)
        return src.scaled(targetSize,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    else {
        return Blitz::smoothScaleFilter(src,targetSize,zg->resizeBlur,
                                        rf,Qt::KeepAspectRatio,page,currentPage);
    }
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
  pixSetColormap(pixs, nullptr);
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
    const char* datapath = nullptr;
#ifdef Q_OS_WIN
    QDir appDir(qApp->applicationDirPath());
    QByteArray tesspath = appDir.absoluteFilePath("tessdata").toUtf8();
    datapath = tesspath.constData();
#endif
    if (ocr->Init(datapath,"jpn")) {
        QMessageBox::critical(nullptr,"QManga error",
                              "Could not initialize Tesseract.\n"
                              "Maybe japanese language training data is not installed.");
        return nullptr;
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
