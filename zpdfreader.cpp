#include "zpdfreader.h"

ZPdfReader::ZPdfReader(QObject *parent, QString filename) :
    ZAbstractReader(parent,filename)
{
    doc = NULL;
}

bool ZPdfReader::openFile()
{
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
}

void ZPdfReader::closeFile()
{
    if (!opened)
        return;

    if (doc!=NULL)
        delete doc;
    doc = NULL;

    opened = false;
    sortList.clear();
}

QImage ZPdfReader::loadPage(int num)
{
    QImage res;
    if (!opened || doc==NULL)
        return res;

    if (num<0 || num>=sortList.count()) return res;

    int idx = sortList.at(num).idx;
    Poppler::Page* page = doc->page(idx);
    if (page!=NULL) {
        double dpi = 72.0*((double)zGlobal->pdfRenderWidth)/page->pageSizeF().width();
        res = page->renderToImage(dpi,dpi);
        delete page;
    }
    return res;
}

QImageHash ZPdfReader::loadPages(QIntList nums)
{
    QImageHash hash;
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
