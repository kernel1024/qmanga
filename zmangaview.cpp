#include <QPainter>
#include <QScrollBar>
#include <QApplication>
#include <QMessageBox>
#include <QWheelEvent>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QContextMenuEvent>

#include <limits.h>
#include "zmangaloader.h"
#include "zmangaview.h"
#include "zglobal.h"
#include "mainwindow.h"

ZMangaView::ZMangaView(QWidget *parent) :
    QWidget(parent)
{
    currentPage = 0;
    privPageCount = 0;
    rotation = 0;
    zoomMode = Fit;
    zoomDynamic = false;
    zoomPos = QPoint();
    dragPos = QPoint();
    copyPos = QPoint();
    drawPos = QPoint();
    copySelection = new QRubberBand(QRubberBand::Rectangle,this);
    copySelection->hide();
    copySelection->setGeometry(QRect());
    curPixmap = QPixmap();
    curUmPixmap = QPixmap();
    openedFile = QString();
    zoomAny = -1;
    iCachePixmaps.clear();
    iCacheData.clear();
    pathCache.clear();
    processingPages.clear();
    cacheLoaders.clear();
    lastSizes.clear();
    lastFileSizes.clear();
    setMouseTracking(true);

    setBackgroundRole(QPalette::Dark);
    QPalette p = palette();
    p.setBrush(QPalette::Dark,QBrush(QColor("#000000")));
    setPalette(p);

    connect(this,SIGNAL(changeMangaCover(QString,int)),zg->db,SLOT(sqlChangeFilePreview(QString,int)),Qt::QueuedConnection);

    int cnt = QThread::idealThreadCount()+1;
    if (cnt<2) cnt=2;
    for (int i=0;i<cnt;i++) {
        QThread* th = new QThread();
        ZMangaLoader* ld = new ZMangaLoader();

        connect(ld,SIGNAL(gotPage(QByteArray,int,QString,QUuid)),
                this,SLOT(cacheGotPage(QByteArray,int,QString,QUuid)),Qt::QueuedConnection);
        connect(ld,SIGNAL(gotError(QString)),
                this,SLOT(cacheGotError(QString)),Qt::QueuedConnection);
        connect(ld,SIGNAL(closeFileRequest()),
                this,SLOT(closeFile()));

        connect(this,SIGNAL(cacheOpenFile(QString,int)),ld,SLOT(openFile(QString,int)),Qt::QueuedConnection);
        connect(this,SIGNAL(cacheCloseFile()),ld,SLOT(closeFile()),Qt::QueuedConnection);

        if (i==0) {
            connect(ld,SIGNAL(gotPageCount(int,int)),
                    this,SLOT(cacheGotPageCount(int, int)),Qt::QueuedConnection);
        }
        ZLoaderHelper h = ZLoaderHelper(th,ld);
        ld->threadID = h.id;

        cacheLoaders << h;

        ld->moveToThread(th);
        th->start();
    }

    emit loadedPage(-1,QString());
}

ZMangaView::~ZMangaView()
{
    for (int i=0;i<cacheLoaders.count();i++)
        cacheLoaders.at(i).thread->quit();
    cacheLoaders.clear();
}

void ZMangaView::cacheGetPage(int num)
{
    int idx = 0, jcnt = INT_MAX;
    for (int i=0;i<cacheLoaders.count();i++)
        if (cacheLoaders.at(i).jobCount<jcnt) {
            idx=i;
            jcnt=cacheLoaders.at(i).jobCount;
        }

    cacheLoaders[idx].addJob();
    QMetaObject::invokeMethod(cacheLoaders.at(idx).loader,"getPage",Qt::QueuedConnection,Q_ARG(int,num));
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

void ZMangaView::getPage(int num)
{
    currentPage = num;
    cacheDropUnusable();
    if ((zg->cachePixmaps && iCachePixmaps.contains(currentPage)) ||
            (!zg->cachePixmaps && iCacheData.contains(currentPage)))
        displayCurrentPage();
    cacheFillNearest();
}

void ZMangaView::displayCurrentPage()
{
    if ((zg->cachePixmaps && iCachePixmaps.contains(currentPage)) ||
            (!zg->cachePixmaps && iCacheData.contains(currentPage))) {
        QPixmap p = QPixmap();
        qint64 ffsz = 0;
        if (!zg->cachePixmaps) {
            QByteArray img = iCacheData.value(currentPage);
            ffsz = img.size();
            if (!p.loadFromData(img))
                p = QPixmap();
            img.clear();
        } else {
            p = iCachePixmaps.value(currentPage);
        }
        if (rotation!=0) {
            QMatrix mr;
            mr.rotate(rotation*90.0);
            p = p.transformed(mr,Qt::SmoothTransformation);
        }

        if (!p.isNull()) {
            if (lastFileSizes.count()>10)
                lastFileSizes.removeFirst();
            if (lastSizes.count()>10)
                lastSizes.removeFirst();

            lastFileSizes << ffsz;
            lastSizes << p.size();

            if (lastFileSizes.count()>0 && lastSizes.count()>0) {
                qint64 sum = 0;
                for (int i=0;i<lastFileSizes.count();i++)
                    sum += lastFileSizes.at(i);
                sum = sum / lastFileSizes.count();
                qint64 sumx = 0;
                qint64 sumy = 0;
                for (int i=0;i<lastSizes.count();i++) {
                    sumx += lastSizes.at(i).width();
                    sumy += lastSizes.at(i).height();
                }
                sumx = sumx / lastSizes.count();
                sumy = sumy / lastSizes.count();
                emit averageSizes(QSize(sumx,sumy),sum);
            }
        }

        curUmPixmap = p;
        emit loadedPage(currentPage,pathCache.value(currentPage));
        setFocus();
        redrawPage();
    }
}

void ZMangaView::openFile(QString filename, int page)
{
    iCachePixmaps.clear();
    iCacheData.clear();
    pathCache.clear();
    currentPage = 0;
    curUmPixmap = QPixmap();
    privPageCount = 0;
    redrawPage();
    emit loadedPage(-1,QString());

    emit cacheOpenFile(filename,page);
    if (filename.startsWith(":CLIP:"))
        openedFile = "Clipboard image";
    else
        openedFile = filename;
    processingPages.clear();
}

void ZMangaView::closeFile()
{
    emit cacheCloseFile();
    curPixmap = QPixmap();
    openedFile = QString();
    privPageCount = 0;
    update();
    emit loadedPage(-1,QString());
    processingPages.clear();
}

void ZMangaView::setPage(int page)
{
    if (page<0 || page>=privPageCount) return;
    getPage(page);
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
        drawPos = QPoint(x,y);

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
        if ((iCacheData.contains(currentPage) || iCachePixmaps.contains(currentPage)) && curUmPixmap.isNull()) {
            QPixmap p(":/img/edit-delete.png");
            w.drawPixmap((width()-p.width())/2,(height()-p.height())/2,p);
            w.setPen(QPen(zg->foregroundColor()));
            w.drawText(0,(height()-p.height())/2+p.height()+5,width(),w.fontMetrics().height(),Qt::AlignCenter,
                       tr("Error loading page %1").arg(currentPage+1));
        }
    }
}

void ZMangaView::mouseMoveEvent(QMouseEvent *event)
{
    QScrollBar* vb = scroller->verticalScrollBar();
    QScrollBar* hb = scroller->horizontalScrollBar();
    if ((event->buttons() & Qt::LeftButton)>0) {
        if ((QApplication::keyboardModifiers() & Qt::AltModifier) != 0) {
            if (!dragPos.isNull()) {
                int hlen = hb->maximum()-hb->minimum();
                int vlen = vb->maximum()-vb->minimum();
                int hvsz = width();
                int vvsz = height();
                hb->setValue(hb->value()+hlen*(dragPos.x()-event->pos().x())/hvsz);
                vb->setValue(vb->value()+vlen*(dragPos.y()-event->pos().y())/vvsz);
            }
            dragPos = event->pos();
        } else {
            copySelection->setGeometry(QRect(event->pos(),copyPos).normalized());
        }
    } else {
        if ((QApplication::keyboardModifiers() & Qt::ControlModifier) == 0) {
            if (zoomDynamic) {
                zoomPos = event->pos();
                update();
            } else
                zoomPos = QPoint();
        }
    }
}

void ZMangaView::mousePressEvent(QMouseEvent *event)
{
    if (event->button()==Qt::MiddleButton) {
        emit minimizeRequested();
        event->accept();
    } else if (event->button()==Qt::LeftButton) {
        if ((QApplication::keyboardModifiers() & Qt::AltModifier) == 0) {
            copyPos = event->pos();
            copySelection->setGeometry(copyPos.x(),copyPos.y(),0,0);
            copySelection->show();
        }
    }
}

void ZMangaView::mouseDoubleClickEvent(QMouseEvent *)
{
    copyPos = QPoint();
    emit doubleClicked();
}

void ZMangaView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button()==Qt::LeftButton &&
            !copyPos.isNull() &&
            (QApplication::keyboardModifiers() & Qt::AltModifier) == 0) {
        if (!curPixmap.isNull()) {
            QRect cp = QRect(event->pos(),copyPos).normalized();
            cp.moveTo(cp.x()-drawPos.x(),cp.y()-drawPos.y());
            cp.moveTo(cp.left()*curUmPixmap.width()/curPixmap.width(),
                      cp.top()*curUmPixmap.height()/curPixmap.height());
            cp.setWidth(cp.width()*curUmPixmap.width()/curPixmap.width());
            cp.setHeight(cp.height()*curUmPixmap.height()/curPixmap.height());
            cp = cp.intersected(curUmPixmap.rect());
#ifdef WITH_OCR
            if (ocr!=NULL && cp.width()>10) {
                QImage cpx = curUmPixmap.copy(cp).toImage();
                ocr->SetImage(Image2PIX(cpx));
                char* rtext = ocr->GetUTF8Text();
                QString s = QString::fromUtf8(rtext);
                delete[] rtext;
                QStringList sl = s.split('\n',QString::SkipEmptyParts);
                int maxlen = 0;
                for (int i=0;i<sl.count();i++)
                    if (sl.at(i).length()>maxlen)
                        maxlen = sl.at(i).length();
                if (maxlen<sl.count()) { // vertical kanji block, needs transpose
                    QStringList sl2;
                    sl2.clear();
                    for (int i=0;i<maxlen;i++)
                        sl2 << QString();
                    for (int i=0;i<sl.count();i++)
                        for (int j=0;j<sl.at(i).length();j++)
                            sl2[maxlen-j-1][i]=sl[i][j];
                    sl = sl2;
                }
                if (zg->ocrEditor!=NULL) {
                    zg->ocrEditor->addText(sl);
                    zg->ocrEditor->showWnd();
                }
            }
#endif
        }
    }
    dragPos = QPoint();
    copyPos = QPoint();
    copySelection->hide();
    copySelection->setGeometry(QRect());
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
        case Qt::Key_Escape:
            emit minimizeRequested();
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
    nt = new QAction(QIcon::fromTheme("document-close"),tr("Close manga"),NULL);
    connect(nt,SIGNAL(triggered()),this,SLOT(closeFileCtx()));
    cm.addAction(nt);
    nt = new QAction(QIcon::fromTheme("view-preview"),tr("Set page as cover"),NULL);
    connect(nt,SIGNAL(triggered()),this,SLOT(changeMangaCoverCtx()));
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
    QPageTimer* fTimer = qobject_cast<QPageTimer *>(sender());
    bool fineDraw = (fTimer!=NULL);
    int savedPage = -1;
    if (fineDraw) {
        savedPage = fTimer->savedPage;
        fTimer->deleteLater();
    }

    if (fineDraw && savedPage!=currentPage) return;

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
            if (sz!=ssz) {
                if (fineDraw || !zg->useFineRendering) {
                    curPixmap = resizeImage(curUmPixmap,sz);
                } else {
                    QTimer *tmr = NULL;
                    if (zg->resizeFilter!=Z::Nearest) {
                        tmr = new QPageTimer(this,100,currentPage);
                        connect(tmr,SIGNAL(timeout()),this,SLOT(redrawPage()));
                    }
                    curPixmap = resizeImage(curUmPixmap,sz,true,Z::Nearest);
                    if (tmr!=NULL)
                        tmr->start();
                }
            }
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
    emit minimizeRequested();
}

void ZMangaView::closeFileCtx()
{
    emit closeFileRequested();
}

void ZMangaView::changeMangaCoverCtx()
{
    if (openedFile.isEmpty()) return;
    if (currentPage<0 || currentPage>=privPageCount) return;

    emit changeMangaCover(openedFile,currentPage);
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

void ZMangaView::viewRotateCCW()
{
    rotation--;
    if (rotation<0) rotation = 3;
    displayCurrentPage();
    emit rotationUpdated(rotation*90);
}

void ZMangaView::viewRotateCW()
{
    rotation++;
    if (rotation>3) rotation = 0;
    displayCurrentPage();
    emit rotationUpdated(rotation*90);
}

void ZMangaView::cacheGotPage(const QByteArray &page, const int &num, const QString &internalPath,
                              const QUuid &threadID)
{
    if (cacheLoaders.contains(ZLoaderHelper(threadID))) {
        cacheLoaders[cacheLoaders.indexOf(ZLoaderHelper(threadID))].delJob();
    }
    if (processingPages.contains(num))
        processingPages.removeOne(num);

    if ((zg->cachePixmaps && iCachePixmaps.contains(num)) ||
            (!zg->cachePixmaps && iCacheData.contains(num))) return;

    if (!zg->cachePixmaps)
        iCacheData[num]=page;
    else {
        QPixmap p = QPixmap();
        if (!p.loadFromData(page))
            p = QPixmap();
        iCachePixmaps[num]=p;
    }
    pathCache[num]=internalPath;

    if (num==currentPage)
        displayCurrentPage();
}

void ZMangaView::cacheGotPageCount(const int &num, const int &preferred)
{
    if (privPageCount<=0) {
        privPageCount = num;
        setPage(preferred);
    }
}

void ZMangaView::cacheGotError(const QString &msg)
{
    QMessageBox::critical(this,tr("QManga"),msg);
}

void ZMangaView::cacheDropUnusable()
{
    if (zg->cachePixmaps)
        iCacheData.clear();
    else
        iCachePixmaps.clear();

    QIntList toCache = cacheGetActivePages();
    QIntList cached;
    if (zg->cachePixmaps)
        cached = iCachePixmaps.keys();
    else
        cached = iCacheData.keys();
    for (int i=0;i<cached.count();i++) {
        if (!toCache.contains(cached.at(i))) {
            if (zg->cachePixmaps)
                iCachePixmaps.remove(cached.at(i));
            else
                iCacheData.remove(cached.at(i));
            pathCache.remove(cached.at(i));
        }
    }
}

void ZMangaView::cacheFillNearest()
{
    QIntList toCache = cacheGetActivePages();
    int idx = 0;
    while (idx<toCache.count()) {
        bool contains = false;
        if (zg->cachePixmaps)
            contains = iCachePixmaps.contains(toCache.at(idx));
        else
            contains = iCacheData.contains(toCache.at(idx));
        if (contains)
            toCache.removeAt(idx);
        else
            idx++;
    }

    for (int i=0;i<toCache.count();i++)
        if (!processingPages.contains(toCache.at(i))) {
            processingPages << toCache.at(i);
            cacheGetPage(toCache.at(i));
        }
}

QIntList ZMangaView::cacheGetActivePages()
{
    QIntList l;
    if (currentPage==-1) return l;
    if (privPageCount<=0) {
        l << currentPage;
        return l;
    }

    int cacheRadius = 1;
    if (zg->cacheWidth>=2) {
        if ((zg->cacheWidth % 2)==0)
            cacheRadius = zg->cacheWidth / 2;
        else
            cacheRadius = (zg->cacheWidth+1) / 2;
    }
    for (int i=0;i<cacheRadius;i++) {
        l << i;
        if (!l.contains(privPageCount-i-1))
            l << privPageCount-i-1;
    }
    for (int i=(currentPage-cacheRadius);i<(currentPage+cacheRadius);i++) {
        if (i>=0 && i<privPageCount && !l.contains(i))
            l << i;
    }
    return l;
}
