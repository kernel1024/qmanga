#ifdef WITH_POPPLER

#include <poppler/PDFDocFactory.h>
#include <poppler/goo/GooString.h>
#include <poppler/GlobalParams.h>
#include <poppler/SplashOutputDev.h>
#include <poppler/splash/SplashBitmap.h>

GlobalParams* globalParams = NULL;

#endif

#include <QMutex>
#include "zpdfreader.h"

static QMutex indexerMutex;

ZPdfReader::ZPdfReader(QObject *parent, QString filename) :
    ZAbstractReader(parent,filename)
{
#ifdef WITH_POPPLER
    doc = NULL;
    outDev = NULL;
#endif
}

ZPdfReader::~ZPdfReader()
{
    closeFile();
}

bool ZPdfReader::openFile()
{
    useImageCatalog = false;
    numPages = 0;

#ifdef WITH_POPPLER
    if (opened || globalParams==NULL)
        return false;

    sortList.clear();

    QFileInfo fi(fileName);
    if (!fi.isFile() || !fi.isReadable()) {
        return false;
    }

    indexerMutex.lock();

    GooString fname(fileName.toUtf8());
    doc = PDFDocFactory().createPDFDoc(fname);

    if (doc==NULL || !doc->isOk())
        return false;

    outDev = new ZPDFImageOutputDev();

    if (zg->pdfRendering==Z::Autodetect || zg->pdfRendering==Z::ImageCatalog) {
        outDev->imageCounting = true;

        if (outDev->isOk())
            doc->displayPages(outDev, 1, doc->getNumPages(), 72, 72, 0,
                              gTrue, gFalse, gFalse);
    }
    if ((zg->pdfRendering==Z::Autodetect && outDev->getPageCount()==doc->getNumPages()) ||
            (zg->pdfRendering==Z::ImageCatalog && outDev->getPageCount()>0)) {
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

    indexerMutex.unlock();

    qSort(sortList);

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

    if (doc!=NULL)
        delete doc;
    doc = NULL;

    if (outDev!=NULL)
        delete outDev;
    outDev = NULL;
#endif

    opened = false;
    numPages = 0;
    useImageCatalog = false;
    sortList.clear();
}

QByteArray ZPdfReader::loadPage(int num)
{
    QByteArray res;
    res.clear();
#ifdef WITH_POPPLER
    if (!opened || doc==NULL || globalParams==NULL)
        return res;

    if (num<0 || num>=sortList.count()) return res;

    int idx = sortList.at(num).idx;

    if (useImageCatalog) {
        BaseStream *str = doc->getBaseStream();
        Goffset sz = outDev->getPage(idx).size;
        res.fill('\0',sz);
        str->setPos(outDev->getPage(idx).pos);
        str->doGetChars(sz,(Guchar *)res.data());
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
        SplashOutputDev splashOutputDev(splashModeXBGR8, 4, gFalse, bgColor, gTrue);
        splashOutputDev.setFontAntialias(gTrue);
        splashOutputDev.setVectorAntialias(gTrue);
        splashOutputDev.setFreeTypeHinting(gTrue, gFalse);
        splashOutputDev.startDoc(doc);
        doc->displayPageSlice(&splashOutputDev, idx + 1,
                              xdpi, ydpi, 0,
                              gFalse, gTrue, gFalse,
                              -1, -1, -1, -1);

        SplashBitmap *bitmap = splashOutputDev.getBitmap();
        const int bw = bitmap->getWidth();
        const int bh = bitmap->getHeight();

        SplashColorPtr data_ptr = bitmap->getDataPtr();

        QImage qimg(reinterpret_cast<uchar *>(data_ptr), bw, bh, QImage::Format_ARGB32);

        QBuffer buf(&res);
        buf.open(QIODevice::WriteOnly);
        qimg.save(&buf,"BMP");
        qimg = QImage();
    }

#endif
    return res;
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
    globalParams = NULL;
}

#else // WITH_POPPLER

void initPdfReader()
{
}

void freePdfReader()
{
}
#endif
