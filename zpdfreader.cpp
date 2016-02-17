#ifdef WITH_POPPLER

#include <poppler/PDFDocFactory.h>
#include <poppler/goo/GooString.h>
#include <poppler/GlobalParams.h>

GlobalParams* globalParams = NULL;

#endif

#include "zpdfreader.h"
#include "ArthurOutputDev.h"


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
    if (zg->pdfRendering==Z::Autodetect && outDev->getPageCount()==doc->getNumPages()) {
        useImageCatalog = true;
        numPages = doc->getNumPages();
    }
    if (zg->pdfRendering==Z::ImageCatalog && outDev->getPageCount()>0) {
        useImageCatalog = true;
        numPages = outDev->getPageCount();
    }

    // TODO: display used method in status bar / message log
    // TODO: add pdfRendering to settings dialog

    outDev->imageCounting = false;

    for (int i=0;i<numPages;i++) {
        sortList << ZFileEntry(QString("%1").arg(i,6,10,QChar('0')),i);
    }

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
        // TODO: check and debug this method!!!
        // painter->setRenderHint(QPainter::Antialiasing);
        // painter->setRenderHint(QPainter::TextAntialiasing);
        QSize size = pageSize(num+1);
        QImage qimg(size.width(), size.height(), QImage::Format_ARGB32);

        QPainter p(&qimg);
        ArthurOutputDev arthur_output(&p);
        arthur_output.startDoc(doc->getXRef());
        doc->displayPage(&arthur_output,idx,zg->dpiX,zg->dpiY,0,
                         false,true,false);
        p.end();
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

QSizeF ZPdfReader::pageSizeF(int page) const
{
    int orient = doc->getPageRotate(page);
    if ( ( 90 == orient ) || ( 270 == orient ) ) {
        return QSizeF( doc->getPageCropHeight(page), doc->getPageCropWidth(page) );
    } else {
        return QSizeF( doc->getPageCropWidth(page), doc->getPageCropHeight(page) );
    }
}

QSize ZPdfReader::pageSize(int page) const
{
    return pageSizeF(page).toSize();
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
