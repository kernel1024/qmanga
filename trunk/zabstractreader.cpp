#include "zabstractreader.h"

ZAbstractReader::ZAbstractReader(QObject *parent, QString filename) :
    QObject(parent)
{
    opened = false;
    fileName = filename;
    pageCount = -1;
}

ZAbstractReader::~ZAbstractReader()
{
    if (opened)
        closeFile();
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
        pageCount = getPageCountPrivate();
    return pageCount;
}

void ZAbstractReader::closeFile()
{
}
