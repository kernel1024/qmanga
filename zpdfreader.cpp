#ifdef WITH_POPPLER

#include <poppler/PDFDocFactory.h>
#include <poppler/goo/GooString.h>
#include <poppler/GlobalParams.h>
#include <poppler/SplashOutputDev.h>
#include <poppler/splash/SplashBitmap.h>
#include <poppler/GlobalParams.h>
#include <poppler/Object.h>
#include <poppler/Dict.h>
#include <poppler/Catalog.h>
#include <poppler/Page.h>
#include <poppler/PDFDoc.h>
#include <poppler/Error.h>

#endif

extern "C" {
#include <zlib.h>
}

#include <QBuffer>
#include <QMutexLocker>
#include <QDebug>
#include "zpdfreader.h"
#include "global.h"
#include "zdb.h"

ZPdfReader::ZPdfReader(QObject *parent, const QString &filename) :
    ZAbstractReader(parent,filename)
{
#ifdef WITH_POPPLER
    m_doc = nullptr;
#endif
}

ZPdfReader::~ZPdfReader()
{
    closeFile();
}

int ZPdfReader::zlibInflate(const char* src, int srcSize,
                                     uchar *dst, int dstSize)
{
    z_stream strm;
    int ret;

    strm.zalloc = nullptr;
    strm.zfree = nullptr;
    strm.opaque = nullptr;
    strm.avail_in = static_cast<uint>(srcSize);
    strm.next_in = reinterpret_cast<uchar*>(const_cast<char*>(src));

    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return -1;

    strm.avail_out = static_cast<uint>(dstSize);
    strm.next_out = dst;
    ret = inflate(&strm,Z_NO_FLUSH);
    inflateEnd(&strm);

    if ((ret != Z_OK) && (ret != Z_STREAM_END)) {
        return -2;
    }

    if ((ret == Z_OK) && (strm.avail_out == 0)) {
        return -3;
    }

    return (dstSize - static_cast<int>(strm.avail_out));
}

bool ZPdfReader::openFile()
{
    QMutexLocker locker(&m_indexerMutex);

    m_useImageCatalog = false;
    m_images.clear();

#ifdef WITH_POPPLER
    if (isOpened() || globalParams==nullptr)
        return false;

    QString fileName = getFileName();
    QFileInfo fi(fileName);
    if (!fi.isFile() || !fi.isReadable()) {
        return false;
    }

    GooString fname(fileName.toUtf8());
    m_doc = PDFDocFactory().createPDFDoc(fname);

    if (m_doc==nullptr || !m_doc->isOk())
        return false;

    Z::PDFRendering mode = zF->global()->db()->getPreferredRendering(fileName);
    if (mode==Z::pdfAutodetect)
        mode = zF->global()->getPdfRendering();

    if (mode==Z::pdfAutodetect || mode==Z::pdfImageCatalog) {

        for (int pageNum=1;pageNum<=m_doc->getNumPages();pageNum++) {
            Dict *dict = m_doc->getPage(pageNum)->getResourceDict();
            if (dict->lookup("XObject").isDict()) {
                Dict *xolist = dict->lookup("XObject").getDict();

                for (int xo_idx=0;xo_idx<xolist->getLength();xo_idx++) {
                    Object stype;
                    Object xitem = xolist->getVal(xo_idx);
                    if (!xitem.isStream()) continue;
                    if (!xitem.streamGetDict()->lookup("Subtype").isName("Image")) continue;

                    BaseStream* data = xitem.getStream()->getBaseStream();
                    int pos = static_cast<int>(data->getStart());
                    int size = static_cast<int>(data->getLength());

                    StreamKind kind = xitem.getStream()->getKind();
                    if (kind==StreamKind::strFlate && // zlib stream
                            xitem.streamGetDict()->lookup("Width").isInt() &&
                            xitem.streamGetDict()->lookup("Height").isInt() &&
                            xitem.streamGetDict()->lookup("BitsPerComponent").isInt()) {
                        int dwidth = xitem.streamGetDict()->lookup("Width").getInt();
                        int dheight = xitem.streamGetDict()->lookup("Height").getInt();
                        int dBPP = xitem.streamGetDict()->lookup("BitsPerComponent").getInt();

                        constexpr int oneBytePerComponent = 8;
                        if (dBPP == oneBytePerComponent) {
                            m_images << ZPDFImg(pos,size,dwidth,dheight,Z::imgFlate);
                        }

                    } else if (kind==StreamKind::strDCT) { // JPEG stream
                        m_images << ZPDFImg(pos,size,0,0,Z::imgDCT);

                    }
                }
            }
        }
    }

    int numPages;
    if ((mode==Z::pdfAutodetect && m_images.count()==m_doc->getNumPages()) ||
            (mode==Z::pdfImageCatalog && m_images.count()>0)) {
        m_useImageCatalog = true;
        numPages = m_images.count();
        postMessage(tr("PDF images catalog"));
    } else {
        m_useImageCatalog = false;
        numPages = m_doc->getNumPages();
        postMessage(tr("PDF renderer"));
    }

    constexpr int pageCounterWidth = 6;
    constexpr int pageCounterBase = 10;
    for (int i=0;i<numPages;i++) {
        addSortEntry(ZFileEntry(QSL("%1").arg(i,pageCounterWidth,
                                                         pageCounterBase,QChar('0')),i));
    }

    performListSort();
    setOpenFileSuccess();

    return true;
#else
    return false;
#endif
}

void ZPdfReader::closeFile()
{
    if (!isOpened())
        return;

#ifdef WITH_POPPLER
    delete m_doc;
    m_doc = nullptr;
#endif

    m_images.clear();
    m_useImageCatalog = false;
    ZAbstractReader::closeFile();
}

#ifdef WITH_POPPLER

void popplerSplashCleanup(void *info)
{
    auto bitmap = reinterpret_cast<SplashBitmap *>(info);
    delete bitmap;
}

void ZPdfReader::loadPagePrivate(int num, QByteArray *buf, QImage *img, bool preferImage)
{
    if (!isOpened() || m_doc==nullptr || globalParams==nullptr)
        return;

    int idx = getSortEntryIdx(num);
    if (idx<0) {
        qCritical() << "Incorrect page number";
        return;
    }

    if (m_useImageCatalog) {
        BaseStream *str = m_doc->getBaseStream();
        const ZPDFImg p = m_images.at(idx);
        Goffset sz = p.size;
        buf->resize(static_cast<int>(sz));
        str->setPos(p.pos);
#ifdef JPDF_PRE073_API
        str->doGetChars(static_cast<int>(sz),reinterpret_cast<Guchar *>(buf->data()));
#else
        str->doGetChars(static_cast<int>(sz),reinterpret_cast<uchar*>(buf->data()));
#endif
        if (p.format == Z::imgDCT) {
            if (preferImage) {
                img->loadFromData(*buf);
            } // else we already have jpeg in buf

        } else if (p.format == Z::imgFlate) {
            *img = QImage(p.width,p.height,QImage::Format_RGB888);
            int sz = zlibInflate((*buf).constData(),(*buf).size(),
                                  (*img).bits(),static_cast<int>((*img).sizeInBytes()));
            (*buf).clear();

            if (sz < 0) {
                qDebug() << "Failed to uncompress page from pdf stream " << num << ". Error: " << sz;
                return;
            }

            if (!preferImage) {
                QBuffer b(buf);
                b.open(QIODevice::WriteOnly);
                (*img).save(&b,"BMP");
                (*img) = QImage();
            } // else we already have img
        } else {
            (*buf).clear();
            qDebug() << "Unknown image format at page from pdf stream " << num;
            return;
        }
    } else {
        qreal xdpi = zF->global()->getDpiX();
        qreal ydpi = zF->global()->getDpiY();
        qreal forceDPI = zF->global()->getForceDPI();
        if (forceDPI>0.0) {
            xdpi = forceDPI;
            ydpi = forceDPI;
        }
        const quint8 cRed = 0xff;
        const quint8 cGreen = 0xff;
        const quint8 cBlue = 0xff;
        const quint8 cAlpha = 0;
        SplashColor bgColor { cRed, cGreen, cBlue, cAlpha };
        SplashOutputDev splashOutputDev(splashModeXBGR8, 4, false, bgColor, true);
        splashOutputDev.setFontAntialias(true);
        splashOutputDev.setVectorAntialias(true);
        splashOutputDev.setFreeTypeHinting(true, false);
        splashOutputDev.startDoc(m_doc);
        m_doc->displayPageSlice(&splashOutputDev, idx + 1,
                              xdpi, ydpi, 0,
                              false, true, false,
                              -1, -1, -1, -1);

        SplashBitmap *bitmap = splashOutputDev.takeBitmap();

        int bw = bitmap->getWidth();
        int bh = bitmap->getHeight();
        QImage qimg(reinterpret_cast<uchar *>(bitmap->getDataPtr()), bw, bh, QImage::Format_ARGB32, popplerSplashCleanup, bitmap);

        if (!preferImage) {
            QBuffer b(buf);
            if (b.open(QIODevice::WriteOnly))
                qimg.save(&b,"BMP");
            qimg = QImage();
        } else
            *img = qimg;
    }
}
#endif

QByteArray ZPdfReader::loadPage(int num)
{
#ifdef WITH_POPPLER
    QByteArray buf;
    QImage img;
    loadPagePrivate(num,&buf,&img,false);
    return buf;
#else
    Q_UNUSED(num)
    return QByteArray();
#endif
}

QImage ZPdfReader::loadPageImage(int num)
{
#ifdef WITH_POPPLER
    QByteArray buf;
    QImage img;
    loadPagePrivate(num,&buf,&img,true);
    return img;
#else
    Q_UNUSED(num)
    return QImage();
#endif
}

QString ZPdfReader::getMagic()
{
    return QSL("PDF");
}

ZPDFImg::ZPDFImg(const ZPDFImg &other)
{
    pos = other.pos;
    size = other.size;
    width = other.width;
    height = other.height;
    format = other.format;
}

ZPDFImg::ZPDFImg(int a_pos, int a_size, int a_width, int a_height, Z::PDFImageFormat a_format)
{
    pos = a_pos;
    size = a_size;
    width = a_width;
    height = a_height;
    format = a_format;
}

bool ZPDFImg::operator==(const ZPDFImg &ref) const
{
    return (pos==ref.pos && size==ref.size && format==ref.format);
}

bool ZPDFImg::operator!=(const ZPDFImg &ref) const
{
    return (pos!=ref.pos || size!=ref.size || format!=ref.format);
}

ZPdfController::ZPdfController(QObject *parent)
    : QObject(parent)
{
#ifdef WITH_POPPLER
    globalParams = new GlobalParams();
#endif
}

ZPdfController::~ZPdfController()
{
#ifdef WITH_POPPLER
    delete globalParams;
    globalParams = nullptr;
#endif
}
