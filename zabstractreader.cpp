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

void ZAbstractReader::postMessage(const QString &msg)
{
    auto *loader = qobject_cast<ZMangaLoader *>(parent());
    if (loader)
        loader->postMessage(msg);
}

void ZAbstractReader::addSortEntry(const QString &name, int idx)
{
    m_sortList.append(qMakePair(name,idx));
}

void ZAbstractReader::performListSort()
{
    std::sort(m_sortList.begin(),m_sortList.end(),[](const ZFileEntry &f1, const ZFileEntry &f2){
        QFileInfo fi1(f1.first);
        QFileInfo fi2(f2.first);
        if (fi1.path()==fi2.path())
            return (zF->compareWithNumerics(fi1.completeBaseName(),fi2.completeBaseName())<0);
        return (zF->compareWithNumerics(fi1.path(),fi2.path())<0);
    });
}

int ZAbstractReader::getSortEntryIdx(int num) const
{
    if (num>=0 && num<m_sortList.count())
        return m_sortList.at(num).second;

    return -1;
}

QString ZAbstractReader::getSortEntryName(int num) const
{
    if (num>=0 && num<m_sortList.count())
        return m_sortList.at(num).first;

    return QString();
}

void ZAbstractReader::setOpenFileSuccess()
{
    m_opened = true;
}
