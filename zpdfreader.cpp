#include "zpdfreader.h"

ZPdfReader::ZPdfReader(QObject *parent, QString filename) :
    ZAbstractReader(parent,filename)
{
#ifdef WITH_POPPLER
    doc = NULL;
#endif
}

bool ZPdfReader::openFile()
{
#ifdef WITH_POPPLER
    if (opened)
        return false;

    sortList.clear();

    QFileInfo fi(fileName);
    if (!fi.isFile() || !fi.isReadable()) {
        return false;
    }

    doc = Poppler::Document::load(fileName);

    if (doc==NULL)
        return false;

    for (int i=0;i<doc->numPages();i++) {
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
#endif

    opened = false;
    sortList.clear();
}

QByteArray ZPdfReader::loadPage(int num)
{
    QByteArray res;
#ifdef WITH_POPPLER
    if (!opened || doc==NULL)
        return res;

    if (num<0 || num>=sortList.count()) return res;

    int idx = sortList.at(num).idx;
    Poppler::Page* page = doc->page(idx);
    if (page!=NULL) {
        double dpi = 72.0*((double)zg->pdfRenderWidth)/page->pageSizeF().width();
        QImage img = page->renderToImage(dpi,dpi);
        delete page;
        QBuffer buf(&res);
        buf.open(QIODevice::WriteOnly);
        img.save(&buf,"BMP");
        img = QImage();
    }
#endif
    return res;
}

QByteHash ZPdfReader::loadPages(QIntList nums)
{
    QByteHash hash;
    if (!opened)
        return hash;

    for (int i=0;i<nums.count();i++)
        hash[nums.at(i)] = loadPage(nums.at(i));

    return hash;
}

QString ZPdfReader::getMagic()
{
    return QString("PDF");
}
