#include "zmangaview.h"
#include "zzipreader.h"
#include "zglobal.h"

ZMangaView::ZMangaView(QWidget *parent) :
    QWidget(parent)
{
    currentPage = 0;
    mReader = NULL;
    zoomMode = Fit;
    currentPixmap = QPixmap();
    setBackgroundRole(QPalette::Dark);
    emit loadPage(-1);
}

ZMangaView::~ZMangaView()
{
    if (mReader!=NULL)
        closeFile();
}

void ZMangaView::setZoomMode(ZMangaView::ZoomMode mode)
{
    zoomMode = mode;
    redrawPage();
}

ZMangaView::ZoomMode ZMangaView::getZoomMode()
{
    return zoomMode;
}

int ZMangaView::getPageCount()
{
    if (!mReader) return -1;
    if (!mReader->isOpened()) return -1;
    return mReader->getPageCount();
}

void ZMangaView::openFile(QString filename)
{
    if (mReader!=NULL)
        closeFile();

    ZAbstractReader* za = readerFactory(filename);
    if (za == NULL) {
        QMessageBox::critical(this,tr("QManga"),tr("File format not supported"));
        return;
    }
    if (!za->openFile()) {
        QMessageBox::critical(this,tr("QManga"),tr("Unable to open file"));
        za->setParent(NULL);
        delete za;
        return;
    }
    mReader = za;
    setPage(0);
}

void ZMangaView::closeFile()
{
    if (mReader!=NULL) {
        mReader->closeFile();
    }
    mReader->setParent(NULL);
    delete mReader;
    mReader = NULL;
    emit loadPage(-1);
}

void ZMangaView::setPage(int page)
{
    if (!mReader) {
        emit loadPage(-1);
        return;
    }
    if (!mReader->isOpened()) {
        emit loadPage(-1);
        return;
    }
    if (page<0 || page>=mReader->getPageCount()) return;

    currentPage = page;
    redrawPage();
    emit loadPage(page);
}

void ZMangaView::wheelEvent(QWheelEvent *event)
{
    int numDegrees = event->delta() / 8;
    int numSteps = numDegrees / 15;

    setPage(currentPage-numSteps);
    event->accept();
}

void ZMangaView::paintEvent(QPaintEvent *)
{
    QPainter w(this);
    if (!currentPixmap.isNull()) {
        int scb = 10;
        if (scroller->verticalScrollBar() && scroller->verticalScrollBar()->isVisible()) {
            scb = scroller->verticalScrollBar()->width();
        } else if (scroller->horizontalScrollBar() && scroller->horizontalScrollBar()->isVisible()) {
            scb = scroller->horizontalScrollBar()->height();
        }

        int x=scb/3;
        int y=scb/3;
        if (currentPixmap.width()<scroller->size().width()-2*scb)
            x=(scroller->size().width()-currentPixmap.width()-2*scb)/2;
        if (currentPixmap.height()<scroller->size().height()-scb)
            y=(scroller->size().height()-currentPixmap.height()-scb)/2;
        w.drawPixmap(x,y,currentPixmap);
    }
}

void ZMangaView::redrawPage()
{
    if (!mReader) return;
    if (!mReader->isOpened()) return;

    // Cache management
    cacheDropUnusable();
    cacheFillNearest();
    int scb = 10;
    if (scroller->verticalScrollBar() && scroller->verticalScrollBar()->isVisible()) {
        scb = scroller->verticalScrollBar()->width();
    } else if (scroller->horizontalScrollBar() && scroller->horizontalScrollBar()->isVisible()) {
        scb = scroller->horizontalScrollBar()->height();
    }

    // Draw current page
    currentPixmap = QPixmap();

    if (iCache.contains(currentPage)) {
        QPixmap p = QPixmap::fromImage(iCache.value(currentPage));
        double pixAspect = (double)p.width()/(double)p.height();
        double myAspect = (double)width()/(double)height();
        QSize sz(scroller->size().width()-scb*2,scroller->size().height()-scb);
        QSize ssz = sz;
        if (zoomMode==Fit) {
            if ( pixAspect > myAspect ) // the image is wider than widget -> resize by width
                sz.setHeight(round(((double)sz.width())/pixAspect));
            else
                sz.setWidth(round(((double)sz.height())*pixAspect));
        } else if (zoomMode==Width) {
            sz.setHeight(round(((double)sz.width())/pixAspect));
        } else if (zoomMode==Height) {
            sz.setWidth(round(((double)sz.height())*pixAspect));
        }
        if (sz!=ssz)
            p = zGlobal->resizeImage(p,sz);
        currentPixmap = p;
        setMinimumSize(currentPixmap.width(),currentPixmap.height());

        if (sz.height()<ssz.height()) sz.setHeight(ssz.height());
        if (sz.width()<ssz.width()) sz.setWidth(ssz.width());
        resize(sz);
        //scroller->ensureVisible(currentPixmap.width()/2,currentPixmap.height()/2,
        //                        scroller->width()/2,scroller->height()/2);
    }
    update();
}

void ZMangaView::navFirst()
{
    if (!mReader) return;
    if (!mReader->isOpened()) return;

    setPage(0);
}

void ZMangaView::navPrev()
{
    if (!mReader) return;
    if (!mReader->isOpened()) return;

    if (currentPage>0)
        setPage(currentPage-1);
}

void ZMangaView::navNext()
{
    if (!mReader) return;
    if (!mReader->isOpened()) return;

    if (currentPage<mReader->getPageCount()-1)
        setPage(currentPage+1);
}

void ZMangaView::navLast()
{
    if (!mReader) return;
    if (!mReader->isOpened()) return;

    setPage(mReader->getPageCount()-1);
}

void ZMangaView::setZoomFit()
{
    setZoomMode(Fit);
}

void ZMangaView::setZoomWidth()
{
    setZoomMode(Width);
}

void ZMangaView::setZoomHeight()
{
    setZoomMode(Height);
}

void ZMangaView::setZoomOriginal()
{
    setZoomMode(Original);
}

void ZMangaView::setZoomDynamic(bool state)
{
    if (state) { };
}

ZAbstractReader* ZMangaView::readerFactory(QString filename)
{
    QString mime = zGlobal->detectMIME(filename).toLower();
    if (mime.contains("application/zip",Qt::CaseInsensitive)) {
        return new ZZipReader(this,filename);
    } else {
        return NULL;
    }
}

void ZMangaView::cacheDropUnusable()
{
    QIntList toCache = cacheGetActivePages();
    QIntList cached = iCache.keys();
    for (int i=0;i<cached.count();i++) {
        if (!toCache.contains(i))
            iCache.remove(i);
    }
}

void ZMangaView::cacheFillNearest()
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

QIntList ZMangaView::cacheGetActivePages()
{
    QIntList l;
    if (!mReader) return l;
    if (!mReader->isOpened()) return l;

    for (int i=0;i<zGlobal->cacheWidth-1;i++) {
        l << i;
        if (!l.contains(mReader->getPageCount()-i-1))
            l << mReader->getPageCount()-i-1;
    }
    for (int i=(currentPage-zGlobal->cacheWidth);i<(currentPage+zGlobal->cacheWidth);i++) {
        if (i>=0 && !l.contains(i))
            l << i;
    }
    return l;
}
