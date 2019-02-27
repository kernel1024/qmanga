#include "zabstractreader.h"
#include "zmangaloader.h"

ZAbstractReader::ZAbstractReader(QObject *parent, const QString &filename) :
    QObject(parent)
{
    opened = false;
    fileName = filename;
    pageCount = -1;
    sortList.clear();
}

ZAbstractReader::~ZAbstractReader()
{
    if (opened)
        closeFile();
    sortList.clear();
}

bool ZAbstractReader::openFile(const QString &filename)
{
    if (opened)
        closeFile();

    fileName = filename;
    pageCount = -1;
    return openFile();
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

void ZAbstractReader::postMessage(const QString &msg)
{
    auto loader = qobject_cast<ZMangaLoader *>(parent());
    if (loader)
        loader->postMessage(msg);
}
