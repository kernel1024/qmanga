#include "zmangacache.h"

ZMangaCache::ZMangaCache(QObject *parent) :
    QObject(parent)
{
    iCache.clear();
    mReader = NULL;
    currentPage = -1;
}

void ZMangaCache::openFile(QString filename, int preferred)
{
    if (mReader!=NULL)
        closeFile();

    ZAbstractReader* za = readerFactory(this,filename);
    if (za == NULL) {
        emit gotError(tr("File format not supported"));
        return;
    }
    if (!za->openFile()) {
        emit gotError(tr("Unable to open file"));
        za->setParent(NULL);
        delete za;
        return;
    }
    mReader = za;
    iCache.clear();
    int pagecnt = mReader->getPageCount();
    emit gotPageCount(pagecnt,preferred);
}

void ZMangaCache::getPage(int num)
{
    currentPage = num;
    cacheDropUnusable();
    cacheFillNearest();

    QImage img;
    int pageNum = num;
    if (iCache.contains(pageNum))
        img = iCache.value(pageNum);
    QString ipt = mReader->getInternalPath(pageNum);

    emit gotPage(img,pageNum,ipt);
}

void ZMangaCache::closeFile()
{
    if (mReader!=NULL) {
        mReader->closeFile();
    }
    mReader->setParent(NULL);
    delete mReader;
    mReader = NULL;
    currentPage = -1;
    iCache.clear();
}

void ZMangaCache::cacheDropUnusable()
{
    QIntList toCache = cacheGetActivePages();
    QIntList cached = iCache.keys();
    for (int i=0;i<cached.count();i++) {
        if (!toCache.contains(i))
            iCache.remove(i);
    }
}

void ZMangaCache::cacheFillNearest()
{
    QIntList toCache = cacheGetActivePages();
    int idx = 0;
    while (idx<toCache.count()) {
        if (iCache.contains(toCache.at(idx)))
            toCache.removeAt(idx);
        else
            idx++;
    }

    iCache.unite(mReader->loadPages(toCache));
}

QIntList ZMangaCache::cacheGetActivePages()
{
    QIntList l;
    if (!mReader) return l;
    if (!mReader->isOpened()) return l;
    if (currentPage==-1) return l;

    for (int i=0;i<zGlobal->cacheWidth-1;i++) {
        l << i;
        if (!l.contains(mReader->getPageCount()-i-1))
            l << mReader->getPageCount()-i-1;
    }
    for (int i=(currentPage-zGlobal->cacheWidth);i<(currentPage+zGlobal->cacheWidth);i++) {
        if (i>=0 && i<mReader->getPageCount() && !l.contains(i))
            l << i;
    }
    return l;
}
