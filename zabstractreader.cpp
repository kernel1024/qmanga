#include <algorithm>
#include "zabstractreader.h"
#include "zmangaloader.h"

ZAbstractReader::ZAbstractReader(QObject *parent, const QString &filename) :
    QObject(parent)
{
    m_fileName = filename;
}

ZAbstractReader::~ZAbstractReader()
{
    if (m_opened)
        closeFile();
    m_sortList.clear();
}

bool ZAbstractReader::openFile(const QString &filename)
{
    if (m_opened)
        closeFile();

    m_fileName = filename;
    m_pageCount = -1;
    m_sortList.clear();
    return openFile();
}

int ZAbstractReader::getPageCount()
{
    if (!m_opened) return -1;
    if (m_pageCount<0)
        m_pageCount = m_sortList.count();
    return m_pageCount;
}

bool ZAbstractReader::isOpened() const
{
    return m_opened;
}

QString ZAbstractReader::getFileName() const
{
    return m_fileName;
}

void ZAbstractReader::closeFile()
{
    if (!isOpened())
        return;

    m_opened = false;
    m_sortList.clear();
}

QString ZAbstractReader::getInternalPath(int idx)
{
    if (!m_opened)
        return QString();

    if (idx>=0 && idx<m_sortList.count())
        return m_sortList.at(idx).name;

    return QString();
}

void ZAbstractReader::postMessage(const QString &msg)
{
    auto *loader = qobject_cast<ZMangaLoader *>(parent());
    if (loader)
        loader->postMessage(msg);
}

void ZAbstractReader::performListSort()
{
    std::sort(m_sortList.begin(),m_sortList.end());
}

void ZAbstractReader::addSortEntry(const ZFileEntry &entry)
{
    m_sortList << entry;
}

int ZAbstractReader::getSortEntryIdx(int num) const
{
    if (num>=0 && num<m_sortList.count())
        return m_sortList.at(num).idx;

    return -1;
}

QString ZAbstractReader::getSortEntryName(int num) const
{
    if (num>=0 && num<m_sortList.count())
        return m_sortList.at(num).name;

    return QString();
}

void ZAbstractReader::setOpenFileSuccess()
{
    m_opened = true;
}
