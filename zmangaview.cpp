#include "zmangaview.h"
#include "zglobal.h"
#include "mainwindow.h"
#include "zmangacache.h"

ZMangaView::ZMangaView(QWidget *parent) :
    QWidget(parent)
{
    currentPage = 0;
    privPageCount = 0;
    zoomMode = Fit;
    zoomDynamic = false;
    zoomPos = QPoint();
    curPixmap = QPixmap();
    curUmPixmap = QPixmap();
    openedFile = QString();
    zoomAny = -1;
    setMouseTracking(true);

    setBackgroundRole(QPalette::Dark);
    QPalette p = palette();
    p.setBrush(QPalette::Dark,QBrush(QColor("#000000")));
    setPalette(p);

    threadCache = new QThread();
    ZMangaCache* iCache = new ZMangaCache();
    connect(iCache,SIGNAL(gotPage(const QImage&,const int&,const QString&)),
            this,SLOT(cacheGotPage(const QImage&,const int&,const QString&)),Qt::QueuedConnection);
    connect(iCache,SIGNAL(gotPageCount(const int&, const int&)),
            this,SLOT(cacheGotPageCount(const int&, const int&)),Qt::QueuedConnection);
    connect(iCache,SIGNAL(gotError(const QString&)),
            this,SLOT(cacheGotError(const QString&)),Qt::QueuedConnection);
    connect(this,SIGNAL(cacheOpenFile(QString,int)),iCache,SLOT(openFile(QString,int)),Qt::QueuedConnection);
    connect(this,SIGNAL(cacheGetPage(int)),iCache,SLOT(getPage(int)),Qt::QueuedConnection);
    connect(this,SIGNAL(cacheCloseFile()),iCache,SLOT(closeFile()),Qt::QueuedConnection);
    iCache->moveToThread(threadCache);
    threadCache->start();

    emit loadedPage(-1,QString());
}

ZMangaView::~ZMangaView()
{
    threadCache->quit();
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
    return privPageCount;
}

void ZMangaView::openFile(QString filename, int page)
{
    emit cacheOpenFile(filename,page);
    openedFile = filename;
}

void ZMangaView::closeFile()
{
    emit cacheCloseFile();
    curPixmap = QPixmap();
    openedFile = QString();
    update();
    emit loadedPage(-1,QString());
}

void ZMangaView::setPage(int page)
{
    if (page<0 || page>=privPageCount) return;
    emit cacheGetPage(page);
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
    if (!curPixmap.isNull()) {
        int scb = 6;
        if (scroller->verticalScrollBar() && scroller->verticalScrollBar()->isVisible()) {
            scb = scroller->verticalScrollBar()->width();
        } else if (scroller->horizontalScrollBar() && scroller->horizontalScrollBar()->isVisible()) {
            scb = scroller->horizontalScrollBar()->height();
        }

        int x=scb/3;
        int y=scb/3;
        if (curPixmap.width()<scroller->size().width()-2*scb)
            x=(scroller->size().width()-curPixmap.width()-2*scb)/2;
        if (curPixmap.height()<scroller->size().height()-2*scb)
            y=(scroller->size().height()-curPixmap.height()-2*scb)/2;
        w.drawPixmap(x,y,curPixmap);

        if (zoomDynamic) {
            QPoint mp(zoomPos.x()-x,zoomPos.y()-y);
            QRect baseRect = curPixmap.rect();
            if (baseRect.contains(mp,true)) {
                mp.setX(mp.x()*curUmPixmap.width()/curPixmap.width());
                mp.setY(mp.y()*curUmPixmap.height()/curPixmap.height());
                int msz = zg->magnifySize;

                if (curPixmap.width()<curUmPixmap.width() || curPixmap.height()<curUmPixmap.height()) {
                    QRect cutBox(mp.x()-msz/2,mp.y()-msz/2,msz,msz);
                    baseRect = curUmPixmap.rect();
                    if (cutBox.left()<baseRect.left()) cutBox.moveLeft(baseRect.left());
                    if (cutBox.right()>baseRect.right()) cutBox.moveRight(baseRect.right());
                    if (cutBox.top()<baseRect.top()) cutBox.moveTop(baseRect.top());
                    if (cutBox.bottom()>baseRect.bottom()) cutBox.moveBottom(baseRect.bottom());
                    QPixmap zoomed = curUmPixmap.copy(cutBox);
                    baseRect = QRect(QPoint(zoomPos.x()-zoomed.width()/2,zoomPos.y()-zoomed.height()/2),
                                     zoomed.size());
                    if (baseRect.left()<0) baseRect.moveLeft(0);
                    if (baseRect.right()>width()) baseRect.moveRight(width());
                    if (baseRect.top()<0) baseRect.moveTop(0);
                    if (baseRect.bottom()>height()) baseRect.moveBottom(height());
                    w.drawPixmap(baseRect,zoomed);
                } else {
                    QRect cutBox(mp.x()-msz/4,mp.y()-msz/4,msz/2,msz/2);
                    baseRect = curUmPixmap.rect();
                    if (cutBox.left()<baseRect.left()) cutBox.moveLeft(baseRect.left());
                    if (cutBox.right()>baseRect.right()) cutBox.moveRight(baseRect.right());
                    if (cutBox.top()<baseRect.top()) cutBox.moveTop(baseRect.top());
                    if (cutBox.bottom()>baseRect.bottom()) cutBox.moveBottom(baseRect.bottom());
                    QPixmap zoomed = curUmPixmap.copy(cutBox);
                    zoomed = resizeImage(zoomed,zoomed.size()*3.0,true,Z::Lanczos);
                    baseRect = QRect(QPoint(zoomPos.x()-zoomed.width()/2,zoomPos.y()-zoomed.height()/2),
                                     zoomed.size());
                    if (baseRect.left()<0) baseRect.moveLeft(0);
                    if (baseRect.right()>width()) baseRect.moveRight(width());
                    if (baseRect.top()<0) baseRect.moveTop(0);
                    if (baseRect.bottom()>height()) baseRect.moveBottom(height());
                    w.drawPixmap(baseRect,zoomed);
                }
            }

        }
    } else {
        if (currentPage>=0 && currentPage<privPageCount && curUmPixmap.isNull()) {
            QPixmap p(":/img/edit-delete.png");
            w.drawPixmap((width()-p.width())/2,(height()-p.height())/2,p);
            w.setPen(QPen(zg->foregroundColor()));
            w.drawText(0,(height()-p.height())/2+p.height()+5,width(),w.fontMetrics().height(),Qt::AlignCenter,
                       tr("Error loading page %1").arg(currentPage));
        }
    }
}

void ZMangaView::mouseMoveEvent(QMouseEvent *event)
{
    if (zoomDynamic) {
        zoomPos = event->pos();
        update();
    } else
        zoomPos = QPoint();
}

void ZMangaView::mouseDoubleClickEvent(QMouseEvent *)
{
    emit doubleClicked();
}

void ZMangaView::keyPressEvent(QKeyEvent *event)
{
    QScrollBar* vb = scroller->verticalScrollBar();
    QScrollBar* hb = scroller->horizontalScrollBar();
    switch (event->key()) {
        case Qt::Key_Up:
            if (vb) vb->setValue(vb->value()-vb->singleStep());
            break;
        case Qt::Key_Down:
            if (vb) vb->setValue(vb->value()+vb->singleStep());
            break;
        case Qt::Key_Left:
            if (hb) hb->setValue(hb->value()-hb->singleStep());
            break;
        case Qt::Key_Right:
            if (hb) hb->setValue(hb->value()+hb->singleStep());
            break;
        case Qt::Key_Space:
        case Qt::Key_PageDown:
            navNext();
            break;
        case Qt::Key_Backspace:
        case Qt::Key_PageUp:
            navPrev();
            break;
        case Qt::Key_Home:
            navFirst();
            break;
        case Qt::Key_End:
            navLast();
            break;
        default:
            emit keyPressed(event->key());
            break;
    }
    event->accept();
}

void ZMangaView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu cm(this);
    QAction* nt = new QAction(QIcon(":/img/zoom-fit-best.png"),tr("Zoom fit"),NULL);
    connect(nt,SIGNAL(triggered()),this,SLOT(setZoomFit()));
    cm.addAction(nt);
    nt = new QAction(QIcon(":/img/zoom-fit-width.png"),tr("Zoom width"),NULL);
    connect(nt,SIGNAL(triggered()),this,SLOT(setZoomWidth()));
    cm.addAction(nt);
    nt = new QAction(QIcon(":/img/zoom-fit-height.png"),tr("Zoom height"),NULL);
    connect(nt,SIGNAL(triggered()),this,SLOT(setZoomHeight()));
    cm.addAction(nt);
    nt = new QAction(QIcon(":/img/zoom-original.png"),tr("Zoom original"),NULL);
    connect(nt,SIGNAL(triggered()),this,SLOT(setZoomOriginal()));
    cm.addAction(nt);
    cm.addSeparator();
    nt = new QAction(QIcon(":/img/zoom-draw.png"),tr("Zoom dynamic"),NULL);
    nt->setCheckable(true);
    nt->setChecked(zoomDynamic);
    connect(nt,SIGNAL(triggered(bool)),this,SLOT(setZoomDynamic(bool)));
    cm.addAction(nt);
    cm.addSeparator();
    nt = new QAction(QIcon::fromTheme("view-refresh"),tr("Redraw page"),NULL);
    connect(nt,SIGNAL(triggered()),this,SLOT(redrawPage()));
    cm.addAction(nt);
    cm.addSeparator();
    nt = new QAction(QIcon::fromTheme("go-down"),tr("Minimize window"),NULL);
    connect(nt,SIGNAL(triggered()),this,SLOT(minimizeWindowCtx()));
    cm.addAction(nt);
    cm.exec(event->globalPos());
    event->accept();
}

void ZMangaView::redrawPage()
{
    QPalette p = palette();
    p.setBrush(QPalette::Dark,QBrush(zg->backgroundColor));
    setPalette(p);

    if (openedFile.isEmpty()) return;
    if (currentPage<0 || currentPage>=privPageCount) return;

    int scb = 6;
    if (scroller->verticalScrollBar() && scroller->verticalScrollBar()->isVisible()) {
        scb = scroller->verticalScrollBar()->width();
    } else if (scroller->horizontalScrollBar() && scroller->horizontalScrollBar()->isVisible()) {
        scb = scroller->horizontalScrollBar()->height();
    }

    // Draw current page
    curPixmap = QPixmap();

    if (!curUmPixmap.isNull()) {
        QSize sz(scroller->size().width()-scb*2,scroller->size().height()-scb*2);
        QSize ssz = sz;
        curPixmap = curUmPixmap;
        if (!curUmPixmap.isNull() && curUmPixmap.height()>0) {
            double pixAspect = (double)curUmPixmap.width()/(double)curUmPixmap.height();
            double myAspect = (double)width()/(double)height();
            if (zoomAny>0) {
                sz = curUmPixmap.size()*((double)(zoomAny)/100.0);
            } else if (zoomMode==Fit) {
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
                curPixmap = resizeImage(curUmPixmap,sz);
        }
        setMinimumSize(curPixmap.width(),curPixmap.height());

        if (sz.height()<ssz.height()) sz.setHeight(ssz.height());
        if (sz.width()<ssz.width()) sz.setWidth(ssz.width());
        resize(sz);
    }
    update();
}

void ZMangaView::ownerResized(const QSize &)
{
    redrawPage();
}

void ZMangaView::minimizeWindowCtx()
{
    emit keyPressed(Qt::Key_F4);
}

void ZMangaView::navFirst()
{
    setPage(0);
}

void ZMangaView::navPrev()
{
    if (currentPage>0)
        setPage(currentPage-1);
}

void ZMangaView::navNext()
{
    if (currentPage<privPageCount-1)
        setPage(currentPage+1);
}

void ZMangaView::navLast()
{
    setPage(privPageCount-1);
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
    zoomDynamic = state;
    redrawPage();
}

void ZMangaView::setZoomAny(QString proc)
{
    zoomAny = -1;
    QString s = proc;
    s.remove(QRegExp("[^0123456789]"));
    bool okconv;
    if (!s.isEmpty()) {
        zoomAny = s.toInt(&okconv);
        if (!okconv)
            zoomAny = -1;
    }
    redrawPage();
}

void ZMangaView::cacheGotPage(const QImage &page, const int &num, const QString &internalPath)
{
    currentPage = num;
    curUmPixmap = QPixmap::fromImage(page);
    emit loadedPage(num,internalPath);
    setFocus();
    redrawPage();
}

void ZMangaView::cacheGotPageCount(const int &num, const int &preferred)
{
    privPageCount = num;
    setPage(preferred);
}

void ZMangaView::cacheGotError(const QString &msg)
{
    QMessageBox::critical(this,tr("QManga"),msg);
}
