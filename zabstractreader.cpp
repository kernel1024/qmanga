#include "zabstractreader.h"

ZAbstractReader::ZAbstractReader(QObject *parent, QString filename) :
    QObject(parent)
{
    opened = false;
    fileName = filename;
    pageCount = -1;
    sortList.clear();
    supportedImg << "jpg" << "jpeg" << "gif" << "png" << "tiff" << "tif" << "bmp";
}

ZAbstractReader::~ZAbstractReader()
{
    if (opened)
        closeFile();
    sortList.clear();
}

bool ZAbstractReader::openFile(QString filename)
{
    if (opened)
        closeFile();

    fileName = filename;
    pageCount = -1;
    return openFile();
}

bool ZAbstractReader::isOpened()
{
    return opened;
}

int ZAbstractReader::getPageCount()
{
    if (!opened) return -1;
    if (pageCount<0)
        pageCount = sortList.count();
    return pageCount;
}

void ZAbstractReader::closeFile()
{
}

QString ZAbstractReader::getInternalPath(int)
{
    return QString();
}
