#include <QMutex>
#include <QBuffer>
#include <QDir>
#include <QApplication>
#include <QSettings>
#include <QAtomicInteger>
#include <QPointer>
#include <QDesktopServices>
#include <QRegularExpression>
#include <iostream>
#include <clocale>
#include <QDebug>

#include "zmangaloader.h"
#include "global.h"
#include "readers/zabstractreader.h"
#include "readers/zzipreader.h"
#include "readers/zrarreader.h"
#include "readers/zpdfreader.h"
#include "readers/zdjvureader.h"
#include "readers/zimagesdirreader.h"
#include "readers/zsingleimagereader.h"
#include "zglobal.h"
#include "zmangaview.h"
#include "scalefilter.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

ZGenericFuncs::ZGenericFuncs(QObject *parent)
    : QObject(parent)
{
}

ZGenericFuncs::~ZGenericFuncs() = default;

ZGenericFuncs *ZGenericFuncs::instance()
{
    static QPointer<ZGenericFuncs> inst;
    static QAtomicInteger<bool> initializedOnce(false);

    if (inst.isNull()) {
        if (initializedOnce.testAndSetAcquire(false,true)) {
            inst = new ZGenericFuncs(QApplication::instance());
            return inst.data();
        }

        qFatal("Accessing to zF after destruction!!!");
        return nullptr;
    }

    return inst.data();
}

void ZGenericFuncs::initialize()
{
#if TESSERACT_MAJOR_VERSION>=4
    setlocale (LC_ALL, "C");
    setlocale (LC_CTYPE, "C");
#endif
    setlocale (LC_NUMERIC, "C");

#ifdef WITH_OCR
    initializeOCR();
#endif
    QGuiApplication::setApplicationDisplayName(QSL("QManga"));
    QCoreApplication::setOrganizationName(QSL("kernel1024"));
    QCoreApplication::setApplicationName(QSL("qmanga"));

    qInstallMessageHandler(ZGenericFuncs::stdConsoleOutput);
    qRegisterMetaType<ZIntVector>("ZIntVector");
    qRegisterMetaType<ZSQLMangaEntry>("ZSQLMangaEntry");
    qRegisterMetaType<ZAlbumEntry>("ZAlbumEntry");
    qRegisterMetaType<ZAlbumVector>("ZAlbumVector");
    qRegisterMetaType<QUuid>("QUuid");
    qRegisterMetaType<Z::Ordering>("Z::Ordering");
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    qRegisterMetaType<ZStrMap>("ZStrMap");
#else
    qRegisterMetaTypeStreamOperators<ZStrMap>("ZStrMap");
#endif
#ifdef WITH_DJVU
    qRegisterMetaType<ZDjVuDocument>("ZDjVuDocument");
#endif

#ifdef Q_OS_WIN
    QDir appDir(getApplicationDirPath());
    QApplication::addLibraryPath(appDir.absoluteFilePath("imageformats"));

    if (QApplication::arguments().contains("--debug"))
        AllocConsole();
#endif

    m_pdfController.reset(new ZPdfController(this));
    m_djvuController.reset(new ZDjVuController(this));
    m_zg.reset(new ZGlobal(this));
}

ZDjVuController *ZGenericFuncs::djvuController() const
{
    return m_djvuController.data();
}

ZGlobal *ZGenericFuncs::global() const
{
    return m_zg.data();
}

ZAbstractReader* ZGenericFuncs::readerFactory(QObject* parent, const QString & filename, bool *mimeOk,
                               bool onlyArchives, bool createReader)
{
    *mimeOk = true;
    if (filename.startsWith(QSL(":CLIP:")) && !onlyArchives) {
        if (createReader)
            return new ZSingleImageReader(parent,filename);

        return nullptr;
    }

    if (filename.startsWith(QSL("#DYN#"))) {
        QString fname(filename);
        fname.remove(QRegularExpression(QSL("^#DYN#")));
        if (createReader)
            return new ZImagesDirReader(parent,fname);

        return nullptr;
    }

    QFileInfo fi(filename);
    if (!fi.exists()) return nullptr;

    QString mime = detectMIME(filename).toLower();
    if (mime.contains(QSL("application/zip"),Qt::CaseInsensitive)) {
        if (createReader)
            return new ZZipReader(parent,filename);

    } else if (mime.contains(QSL("application/x-rar"),Qt::CaseInsensitive) ||
               mime.contains(QSL("application/rar"),Qt::CaseInsensitive)) {
        if (createReader)
            return new ZRarReader(parent,filename);

#ifdef WITH_POPPLER
    } else if (mime.contains(QSL("application/pdf"),Qt::CaseInsensitive)) {
        if (createReader)
            return new ZPdfReader(parent,filename);

#endif
#ifdef WITH_DJVU
    } else if (mime.contains(QSL("image/vnd.djvu"),Qt::CaseInsensitive)) {
        if (createReader)
            return new ZDjVuReader(parent,filename);

#endif
    } else if (mime.contains(QSL("image/"),Qt::CaseInsensitive) && !onlyArchives) {
        if (createReader)
            return new ZSingleImageReader(parent,filename);

    } else {
        *mimeOk = false;
    }
    return nullptr;
}

void ZGenericFuncs::stdConsoleOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    static QMutex loggerMutex;

    loggerMutex.lock();

    QString lmsg = QString();

    int line = context.line;
    QString file = QString::fromUtf8(context.file);
    QString category = QString::fromUtf8(context.category);
    if (category==QSL("default")) {
        category.clear();
    } else {
        category.append(' ');
    }

    switch (type) {
        case QtDebugMsg:
            lmsg = QSL("%1Debug: %2 (%3:%4)").arg(category, msg, file, QSL("%1").arg(line));
            break;
        case QtWarningMsg:
            lmsg = QSL("%1Warning: %2 (%3:%4)").arg(category, msg, file, QSL("%1").arg(line));
            break;
        case QtCriticalMsg:
            lmsg = QSL("%1Critical: %2 (%3:%4)").arg(category, msg, file, QSL("%1").arg(line));
            break;
        case QtFatalMsg:
            lmsg = QSL("%1Fatal: %2 (%3:%4)").arg(category, msg, file, QSL("%1").arg(line));
            break;
        case QtInfoMsg:
            lmsg = QSL("%1Info: %2 (%3:%4)").arg(category, msg, file, QSL("%1").arg(line));
            break;
    }

    if (!lmsg.isEmpty()) {
        QString fmsg = QSL("%1 %2").arg(QTime::currentTime()
                                                   .toString(QSL("h:mm:ss")),lmsg);
        fmsg.append(QSL("\r\n"));

#ifdef Q_OS_WIN
        HANDLE con = GetStdHandle(STD_ERROR_HANDLE);
        wchar_t* wmsg = new wchar_t[fmsg.length()];
        fmsg.toWCharArray(wmsg);
        DWORD wr = 0;
        WriteConsoleW(con,wmsg,fmsg.length(),&wr,NULL);
        delete[] wmsg;
#endif
        std::cerr << fmsg.toLocal8Bit().constData() << std::endl;
    }

    loggerMutex.unlock();
}

QString ZGenericFuncs::formatSize(qint64 size)
{
    const int precision = 2;
    return QLocale::c().formattedDataSize(size,precision,QLocale::DataSizeTraditionalFormat);
}

QString ZGenericFuncs::elideString(const QString& text, int maxlen, Qt::TextElideMode mode)
{
    if (text.length()<maxlen)
        return text;

    if (mode == Qt::ElideRight)
        return QSL("%1...").arg(text.left(maxlen));

    if (mode == Qt::ElideLeft)
        return QSL("...%1").arg(text.right(maxlen));

    if (mode == Qt::ElideMiddle)
        return QSL("%1...%2").arg(text.left(maxlen/2),text.right(maxlen/2));

    return text;
}

QString ZGenericFuncs::escapeParam(const QString &param)
{
    QString res = param;
    res.replace('\'',QSL("\\'"));
    res.replace('"',QSL("\\\""));
    res.replace(';',QString());
    res.replace('%',QString());
    return res;
}

int ZGenericFuncs::compareWithNumerics(const QString &ref1, const QString &ref2)
{
    QStringList r1nums;
    QStringList r1sep;
    QString sn;
    QString sc;
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
            if (ref1[i].isDigit()) {
                sn.append(ref1[i]);
            } else if (ref1[i].isLetter()) {
                sc.append(ref1[i]);
            }
        } else {
            sc.clear();
            sn.clear();
        }
    }
    QStringList r2nums;
    QStringList r2sep;
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
            if (ref2[i].isDigit()) {
                sn.append(ref2[i]);
            } else if (ref2[i].isLetter()) {
                sc.append(ref2[i]);
            }
        } else {
            sc.clear();
            sn.clear();
        }
    }
    //    QStringList r1nums = ref1.split(QRegExp("\\D+"),QString::SkipEmptyParts);
    //    QStringList r2nums = ref2.split(QRegExp("\\D+"),QString::SkipEmptyParts);
    int mlen = qMin(r1nums.count(),r2nums.count());
    for (int i=0;i<mlen;i++) {
        bool okconv = false;
        int r1n = r1nums.at(i).toInt(&okconv);
        if (!okconv) break;
        int r2n = r2nums.at(i).toInt(&okconv);
        if (!okconv) break;

        if (r1sep.at(i)<r2sep.at(i)) return -1;
        if (r1sep.at(i)>r2sep.at(i)) return 1;
        if (r1n<r2n) return -1;
        if (r1n>r2n) return 1;
    }
    return QString::compare(ref1,ref2);
}

QString ZGenericFuncs::getOpenFileNameD ( QWidget * parent, const QString & caption, const QString & dir, const QString & filter, QString * selectedFilter, QFileDialog::Options options )
{
    return QFileDialog::getOpenFileName(parent,caption,dir,filter,selectedFilter,options);
}

QStringList ZGenericFuncs::getOpenFileNamesD ( QWidget * parent, const QString & caption, const QString & dir, const QString & filter, QString * selectedFilter, QFileDialog::Options options )
{
    return QFileDialog::getOpenFileNames(parent,caption,dir,filter,selectedFilter,options);
}

QString	ZGenericFuncs::getExistingDirectoryD ( QWidget * parent, const QString & caption, const QString & dir, QFileDialog::Options options )
{
    return QFileDialog::getExistingDirectory(parent,caption,dir,options);
}

void ZGenericFuncs::showInGraphicalShell(const QString &pathIn)
{
    const QFileInfo fileInfo(pathIn);
    bool success = false;

#ifdef Q_OS_WIN
    const QString app = QStandardPaths::findExecutable(QSL("explorer.exe"));
    if (app.isEmpty()) {
        QDesktopServices::openUrl(pathIn);
        return;
    }
    QStringList param;
    if (!fileInfo.isDir())
        param += QLatin1String("/select,");
    param += QDir::toNativeSeparators(fileInfo.canonicalFilePath());
    success = QProcess::startDetached(app, param));
#else
    QString app;
    QStringList browserArgs;
    if (qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains(QSL("KDE"),Qt::CaseInsensitive)) {
        app = QStandardPaths::findExecutable(QSL("dolphin"));
        if (!fileInfo.isDir())
            browserArgs += QSL("--select");
        browserArgs += pathIn;
    } else if (qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains(QSL("Gnome"),Qt::CaseInsensitive)) {
        app = QStandardPaths::findExecutable(QSL("nautilus"));
        if (!fileInfo.isDir())
            browserArgs += QSL("-s");
        browserArgs += pathIn;
    }
    QProcess browserProc;
    if (!app.isEmpty()) {
        success = browserProc.startDetached(app,browserArgs);
        const QString error = QString::fromLocal8Bit(browserProc.readAllStandardError());
        success = success && error.isEmpty();
    }
#endif
    if (!success)
        QDesktopServices::openUrl(pathIn);
}

QString ZGenericFuncs::detectMIME(const QString& filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly))
        return QSL("text/plain");
    QByteArray ba = f.read(ZDefaults::magicBlockSize);
    f.close();
    return detectMIME(ba);
}

QString ZGenericFuncs::detectMIME(const QByteArray &buf)
{
    static const QHash<QPair<int, QByteArray>, QString> magicList = {
        { { 0, QBAL("\x50\x4b\x03\x04") }, "application/zip" },
        { { 0, QBAL("\x52\x61\x72\x21") }, "application/rar" },
        { { 0, QBAL("\x25\x50\x44\x46") }, "application/pdf" },
        { { 0, QBAL("\xFF\xD8\xFF\xDB") }, "image/jpeg" },
        { { 6, QBAL("\x4A\x46\x49\x46") }, "image/jpeg" },
        { { 6, QBAL("\x45\x78\x69\x66") }, "image/jpeg" },
        { { 0, QBAL("\x89\x50\x4E\x47") }, "image/png" },
        { { 0, QBAL("\x47\x49\x46\x38\x37\x61") }, "image/gif" },
        { { 0, QBAL("\x47\x49\x46\x38\x39\x61") }, "image/gif" },
        { { 0, QBAL("\x49\x49\x2A\x00") }, "image/tiff" },
        { { 0, QBAL("\x4D\x4D\x00\x2A") }, "image/tiff" },
        { { 0, QBAL("\x42\x4D") }, "image/bmp" },
        { { 12, QBAL("\x44\x4A\x56") }, "image/vnd.djvu"}
    };

    for (auto it = magicList.constKeyValueBegin(), end = magicList.constKeyValueEnd(); it != end; ++it) {
        const QByteArray test = buf.mid((*it).first.first,(*it).first.second.size());
        if (test==(*it).first.second)
            return (*it).second;
    }
    return QSL("text/plain");
}

QImage ZGenericFuncs::resizeImage(const QImage& src, const QSize& targetSize, bool forceFilter,
                   Blitz::ScaleFilterType filter, int page, const int *currentPage)
{
    if (!zF->global()) return QImage();

    QSize dsize = src.size().scaled(targetSize,Qt::KeepAspectRatio);

    Blitz::ScaleFilterType rf = zF->global()->getDownscaleFilter();
    if (dsize.width()>src.size().width())
        rf = zF->global()->getUpscaleFilter();

    if (forceFilter)
        rf = filter;

    if (rf==Blitz::UndefinedFilter)
        return src.scaled(targetSize,Qt::IgnoreAspectRatio,Qt::FastTransformation);
    if (rf==Blitz::Bilinear)
        return src.scaled(targetSize,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);

    return Blitz::smoothScaleFilter(src,targetSize,zF->global()->getResizeBlur(),
                                    rf,Qt::KeepAspectRatio,page,currentPage);
}

QStringList ZGenericFuncs::supportedImg()
{
    static const QStringList res { QSL("jpg"), QSL("jpeg"), QSL("jpe"), QSL("gif"), QSL("png"),
                QSL("tiff"), QSL("tif"), QSL("bmp") };
    return res;
}

QFileInfoList ZGenericFuncs::filterSupportedImgFiles(const QFileInfoList& entryList)
{
    QFileInfoList res;
    QStringList suffices = supportedImg();
    for (const auto &fi : entryList) {
        if (suffices.contains(fi.suffix(),Qt::CaseInsensitive))
            res.append(fi);
    }

    return res;
}

#ifdef WITH_OCR
PIX* ZGenericFuncs::Image2PIX(const QImage &qImage) {
    PIX * pixs = nullptr;
    l_uint32 *lines = nullptr;

    QImage img = qImage.rgbSwapped();
    int width = img.width();
    int height = img.height();
    int depth = img.depth();
    int wpl = img.bytesPerLine() / 4;

    pixs = pixCreate(width, height, depth);
    pixSetWpl(pixs, wpl);
    pixSetColormap(pixs, nullptr);
    l_uint32 *datas = pixs->data;

    for (int y = 0; y < height; y++) {
        lines = datas + y * wpl; // NOLINT
        memcpy(lines,img.scanLine(y),static_cast<uint>(img.bytesPerLine()));
    }
    return pixEndianByteSwapNew(pixs);
}

QString ZGenericFuncs::processImageWithOCR(const QImage &image)
{
    m_ocr->SetImage(Image2PIX(image));
    char* rtext = m_ocr->GetUTF8Text();
    QString s = QString::fromUtf8(rtext);
    delete[] rtext;
    return s;
}

bool ZGenericFuncs::isOCRReady() const
{
    return (m_ocr!=nullptr);
}

QString ZGenericFuncs::ocrGetActiveLanguage()
{
    QSettings settings(QSL("kernel1024"), QSL("qmanga-ocr"));
    settings.beginGroup(QSL("Main"));
    QString res = settings.value(QSL("activeLanguage"),QSL("jpn")).toString();
    settings.endGroup();
    return res;
}

QString ZGenericFuncs::ocrGetDatapath()
{
    QSettings settings(QSL("kernel1024"), QSL("qmanga-ocr"));
    settings.beginGroup(QSL("Main"));
#ifdef Q_OS_WIN
    QDir appDir(getApplicationDirPath());
    QString res = settings.value(QStringLiteral("datapath"),
                                 appDir.absoluteFilePath(QStringLiteral("tessdata"))).toString();
#else
    QString res = settings.value(QSL("datapath"),
                                 QSL("/usr/share/tessdata/")).toString();
#endif
    settings.endGroup();
    return res;
}

void ZGenericFuncs::initializeOCR()
{
    m_ocr = new tesseract::TessBaseAPI();
    QByteArray tesspath = ocrGetDatapath().toUtf8();

    QString lang = ocrGetActiveLanguage();
    if (lang.isEmpty() || (m_ocr->Init(tesspath.constData(),lang.toUtf8().constData())!=0)) {
        qCritical() << "Could not initialize Tesseract. "
                       "Maybe language training data is not installed.";
    }
}

#ifdef Q_OS_WIN
QString ZGenericFuncs::getApplicationDirPath()
{
    static QString res {};
    if (!res.isEmpty())
        return res;

    wchar_t path[MAX_PATH];
    size_t sz = GetModuleFileNameW(nullptr,path,MAX_PATH);
    if (sz>0) {
        QFileInfo fi(QString::fromWCharArray(path,static_cast<int>(sz)));
        if (fi.isFile())
            res = fi.absolutePath();
    }
    if (res.isEmpty())
        qFatal("Unable to determine executable path.");
    return res;
}
#endif // WIN32

#endif // OCR

const QHash<Z::Ordering, QString> &ZGenericFuncs::getHeaderColumns()
{
    static const QHash<Z::Ordering,QString> headerColumns = {
        { Z::Ordering::ordName, QSL("Name") },
        { Z::Ordering::ordAlbum, QSL("Album") },
        { Z::Ordering::ordPagesCount, QSL("Pages") },
        { Z::Ordering::ordFileSize, QSL("Size") },
        { Z::Ordering::ordAddingDate, QSL("Added") },
        { Z::Ordering::ordCreationFileDate, QSL("Created") },
        { Z::Ordering::ordMagic, QSL("Type") }
    };
    return headerColumns;
}

const QHash<Z::Ordering, QString> &ZGenericFuncs::getSqlColumns()
{
    static const QHash<Z::Ordering,QString> sqlColumns = {
        { Z::Ordering::ordName, QSL("files.name") },
        { Z::Ordering::ordAlbum, QSL("albums.name") },
        { Z::Ordering::ordPagesCount, QSL("pagesCount") },
        { Z::Ordering::ordFileSize, QSL("fileSize") },
        { Z::Ordering::ordAddingDate, QSL("addingDT") },
        { Z::Ordering::ordCreationFileDate, QSL("fileDT") },
        { Z::Ordering::ordMagic, QSL("fileMagic") }
    };
    return sqlColumns;
}

const QHash<Z::Ordering, QString> &ZGenericFuncs::getSortMenu()
{
    static const QHash<Z::Ordering,QString> sortMenu = {
        { Z::Ordering::ordName, QSL("By name") },
        { Z::Ordering::ordAlbum, QSL("By album") },
        { Z::Ordering::ordPagesCount, QSL("By pages count") },
        { Z::Ordering::ordFileSize, QSL("By file size") },
        { Z::Ordering::ordAddingDate, QSL("By adding date") },
        { Z::Ordering::ordCreationFileDate, QSL("By file creation date") },
        { Z::Ordering::ordMagic, QSL("By file type") }
    };
    return sortMenu;
}

// ZGenericFuncs -----------------------

ZAlbumEntry::ZAlbumEntry(const ZAlbumEntry &other)
{
    id = other.id;
    parent = other.parent;
    name = other.name;
}

ZAlbumEntry::ZAlbumEntry(int aId, int aParent, const QString &aName)
{
    id = aId;
    parent = aParent;
    name = aName;
}

bool ZAlbumEntry::operator==(const ZAlbumEntry &ref) const
{
    return (id == ref.id);
}

bool ZAlbumEntry::operator!=(const ZAlbumEntry &ref) const
{
    return (id != ref.id);
}

ZSQLMangaEntry::ZSQLMangaEntry(const ZSQLMangaEntry &other)
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

ZSQLMangaEntry::ZSQLMangaEntry(int aDbid)
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
    rendering = Z::PDFRendering::pdfAutodetect;
}

ZSQLMangaEntry::ZSQLMangaEntry(const QString &aName, const QString &aFilename, const QString &aAlbum,
                             const QImage &aCover, int aPagesCount, qint64 aFileSize,
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

bool ZSQLMangaEntry::operator ==(const ZSQLMangaEntry &ref) const
{
    return (ref.dbid==dbid);
}

bool ZSQLMangaEntry::operator !=(const ZSQLMangaEntry &ref) const
{
    return (ref.dbid!=dbid);
}

ZLoaderHelper::ZLoaderHelper()
{
    id = QUuid::createUuid();
}

ZLoaderHelper::ZLoaderHelper(const ZLoaderHelper &other)
{
    id = other.id;
    thread = other.thread;
    loader = other.loader;
    jobCount = other.jobCount;
}

ZLoaderHelper::ZLoaderHelper(const QUuid &aThreadID)
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
    loader->m_threadID = id;
}

ZLoaderHelper::ZLoaderHelper(const QPointer<QThread> &aThread, const QPointer<ZMangaLoader> &aLoader)
{
    id = QUuid::createUuid();
    thread = aThread;
    loader = aLoader;
    jobCount = 0;
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
