#ifdef WITH_POPPLER

#include <poppler/PDFDocFactory.h>
#include <poppler/goo/GooString.h>
#include <poppler/GlobalParams.h>
#include <poppler/SplashOutputDev.h>
#include <poppler/splash/SplashBitmap.h>
#include <poppler/GlobalParams.h>

#endif

#include <algorithm>
#include <QBuffer>
#include <QMutexLocker>
#include <QDebug>
#include "zpdfreader.h"
#include "global.h"
#include "zdb.h"
#include "zpdfimageoutdev.h"

ZPdfReader::ZPdfReader(QObject *parent, const QString &filename) :
    ZAbstractReader(parent,filename)
{
#ifdef WITH_POPPLER
    doc = nullptr;
    outDev = nullptr;
#endif
    useImageCatalog = false;
    numPages = 0;
}

ZPdfReader::~ZPdfReader()
{
    closeFile();
}

bool ZPdfReader::openFile()
{
    QMutexLocker locker(&indexerMutex);

    useImageCatalog = false;
    numPages = 0;

#ifdef WITH_POPPLER
    if (opened || globalParams==nullptr)
        return false;

    sortList.clear();

    QFileInfo fi(fileName);
    if (!fi.isFile() || !fi.isReadable()) {
        return false;
    }

    GooString fname(fileName.toUtf8());
    doc = PDFDocFactory().createPDFDoc(fname);

    if (doc==nullptr || !doc->isOk())
        return false;

    outDev = new ZPDFImageOutputDev();

    Z::PDFRendering mode = zg->db->getPreferredRendering(fileName);
    if (mode==Z::Autodetect)
        mode = zg->pdfRendering;

    if (mode==Z::Autodetect || mode==Z::ImageCatalog) {
        outDev->imageCounting = true;

        if (outDev->isOk())
            doc->displayPages(outDev, 1, doc->getNumPages(), 72, 72, 0,
                              true, false, false);
    }
    if ((mode==Z::Autodetect && outDev->getPageCount()==doc->getNumPages()) ||
            (mode==Z::ImageCatalog && outDev->getPageCount()>0)) {
        useImageCatalog = true;
        numPages = outDev->getPageCount();
        emit auxMessage(tr("PDF images catalog"));
    } else {
        useImageCatalog = false;
        numPages = doc->getNumPages();
        emit auxMessage(tr("PDF renderer"));
    }

    outDev->imageCounting = false;

    for (int i=0;i<numPages;i++) {
        sortList << ZFileEntry(QString("%1").arg(i,6,10,QChar('0')),i);
    }

    std::sort(sortList.begin(),sortList.end());

    opened = true;
    return true;
#else
    return false;
#endif
}

void ZPdfReader::closeFile()
{
#ifdef WITH_POPPLER
    if (!opened)
        return;

    delete doc;
    doc = nullptr;

    delete outDev;
    outDev = nullptr;
#endif

    opened = false;
    numPages = 0;
    useImageCatalog = false;
    sortList.clear();
}

#ifdef WITH_POPPLER

void popplerSplashCleanup(void *info)
{
    auto bitmap = reinterpret_cast<SplashBitmap *>(info);
    delete bitmap;
}

void ZPdfReader::loadPagePrivate(int num, QByteArray *buf, QImage *img, bool preferImage)
{
    if (!opened || doc==nullptr || globalParams==nullptr)
        return;

    if (num<0 || num>=sortList.count()) return;

    int idx = sortList.at(num).idx;

    if (useImageCatalog) {
        BaseStream *str = doc->getBaseStream();
        Goffset sz = outDev->getPage(idx).size;
        buf->resize(static_cast<int>(sz));
        str->setPos(outDev->getPage(idx).pos);
#ifdef JPDF_PRE073_API
        str->doGetChars(static_cast<int>(sz),reinterpret_cast<Guchar *>(buf->data()));
#else
        str->doGetChars(static_cast<int>(sz),reinterpret_cast<uchar*>(buf->data()));
#endif
        if (preferImage)
            img->loadFromData(*buf);

    } else {
        qreal xdpi = zg->dpiX;
        qreal ydpi = zg->dpiY;
        if (zg->forceDPI>0.0) {
            xdpi = zg->forceDPI;
            ydpi = zg->forceDPI;
        }
        SplashColor bgColor;
        unsigned int paper_color = 0x0ffffff;
        bgColor[0] = paper_color & 0xff;
        bgColor[1] = (paper_color >> 8) & 0xff;
        bgColor[2] = (paper_color >> 16) & 0xff;
        SplashOutputDev splashOutputDev(splashModeXBGR8, 4, false, bgColor, true);
        splashOutputDev.setFontAntialias(true);
        splashOutputDev.setVectorAntialias(true);
        splashOutputDev.setFreeTypeHinting(true, false);
        splashOutputDev.startDoc(doc);
        doc->displayPageSlice(&splashOutputDev, idx + 1,
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
    return QString("PDF");
}

#ifdef WITH_POPPLER

void initPdfReader()
{
    globalParams = new GlobalParams();
}

void freePdfReader()
{
    delete globalParams;
    globalParams = nullptr;
}

#else // WITH_POPPLER

void initPdfReader()
{
}

void freePdfReader()
{
}
#endif
