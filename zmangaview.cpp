#include <QPainter>
#include <QScrollBar>
#include <QApplication>
#include <QMessageBox>
#include <QWheelEvent>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QContextMenuEvent>
#include <QProgressDialog>
#include <QtConcurrentMap>
#include <QtConcurrentRun>
#include <QFutureWatcher>
#include <QThreadPool>
#include <functional>
#include <limits.h>
#include <math.h>
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
    scrollAccumulator = 0;
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

    connect(this,SIGNAL(changeMangaCover(QString,int)),
            zg->db,SLOT(sqlChangeFilePreview(QString,int)),Qt::QueuedConnection);
    connect(this,SIGNAL(updateFileStats(QString)),
            zg->db,SLOT(sqlUpdateFileStats(QString)),Qt::QueuedConnection);

    int cnt = QThreadPool::globalInstance()->maxThreadCount()+1;
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
        connect(ld,SIGNAL(auxMessage(QString)),
                this,SLOT(loaderMsg(QString)),Qt::QueuedConnection);

        connect(this,SIGNAL(cacheOpenFile(QString,int)),ld,SLOT(openFile(QString,int)),Qt::QueuedConnection);
        connect(this,SIGNAL(cacheCloseFile()),ld,SLOT(closeFile()),Qt::QueuedConnection);

        connect(th,&QThread::finished,ld,&QObject::deleteLater);
        connect(th,&QThread::finished,th,&QObject::deleteLater);

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

    if (scroller->verticalScrollBar()->isVisible())
        scroller->verticalScrollBar()->setValue(0);
    if (scroller->horizontalScrollBar()->isVisible())
        scroller->horizontalScrollBar()->setValue(0);

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
    emit updateFileStats(filename);
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
    scrollAccumulator = 0;
    if (page<0 || page>=privPageCount) return;
    getPage(page);
}

void ZMangaView::wheelEvent(QWheelEvent *event)
{
    int sf = 1;
    int dy = event->angleDelta().y();
    QScrollBar *vb = scroller->verticalScrollBar();
    if (vb!=nullptr && vb->isVisible()) { // page is zoomed and scrollable
        if (((vb->value()>vb->minimum()) && (vb->value()<vb->maximum()))    // somewhere middle of page
                || ((vb->value()==vb->minimum()) && (dy<0))                 // at top, scrolling down
                || ((vb->value()==vb->maximum()) && (dy>0))) {              // at bottom, scrolling up
            scrollAccumulator = 0;
            event->ignore();
            return;
        } else { // at the page border, attempting to flip the page
            sf = zg->scrollFactor;
        }
    }

    if (abs(dy)<zg->detectedDelta)
        zg->detectedDelta = abs(dy);

    scrollAccumulator+= dy;
    int numSteps = scrollAccumulator / (zg->scrollDelta * sf);

    if (numSteps!=0)
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
                    zoomed = QPixmap::fromImage(
                                 resizeImage(zoomed.toImage(),zoomed.size()*3.0,true,zg->upscaleFilter));
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
            QPixmap p(":/32/edit-delete");
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
#ifdef WITH_OCR
            copySelection->setGeometry(QRect(event->pos(),copyPos).normalized());
#endif
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
#ifdef WITH_OCR
            copyPos = event->pos();
            copySelection->setGeometry(copyPos.x(),copyPos.y(),0,0);
            copySelection->show();
#endif
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
#ifdef WITH_OCR
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
            if (ocr!=nullptr && cp.width()>20 && cp.height()>20) {
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
                if (zg->ocrEditor!=nullptr) {
                    zg->ocrEditor->addText(sl);
                    zg->ocrEditor->showWnd();
                }
            }
        }
    }
#endif
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
    QAction* nt = new QAction(QIcon(":/16/zoom-fit-best"),tr("Zoom fit"),nullptr);
    connect(nt,SIGNAL(triggered()),this,SLOT(setZoomFit()));
    cm.addAction(nt);
    nt = new QAction(QIcon(":/16/zoom-fit-width"),tr("Zoom width"),nullptr);
    connect(nt,SIGNAL(triggered()),this,SLOT(setZoomWidth()));
    cm.addAction(nt);
    nt = new QAction(QIcon(":/16/zoom-fit-height"),tr("Zoom height"),nullptr);
    connect(nt,SIGNAL(triggered()),this,SLOT(setZoomHeight()));
    cm.addAction(nt);
    nt = new QAction(QIcon(":/16/zoom-original"),tr("Zoom original"),nullptr);
    connect(nt,SIGNAL(triggered()),this,SLOT(setZoomOriginal()));
    cm.addAction(nt);
    cm.addSeparator();
    nt = new QAction(QIcon(":/16/zoom-draw.png"),tr("Zoom dynamic"),nullptr);
    nt->setCheckable(true);
    nt->setChecked(zoomDynamic);
    connect(nt,SIGNAL(triggered(bool)),this,SLOT(setZoomDynamic(bool)));
    cm.addAction(nt);
    cm.addSeparator();
    nt = new QAction(QIcon(":/16/view-refresh"),tr("Redraw page"),nullptr);
    connect(nt,SIGNAL(triggered()),this,SLOT(redrawPage()));
    cm.addAction(nt);
    cm.addSeparator();
    nt = new QAction(QIcon(":/16/document-close"),tr("Close manga"),nullptr);
    connect(nt,SIGNAL(triggered()),this,SLOT(closeFileCtx()));
    cm.addAction(nt);
    nt = new QAction(QIcon(":/16/view-preview"),tr("Set page as cover"),nullptr);
    connect(nt,SIGNAL(triggered()),this,SLOT(changeMangaCoverCtx()));
    cm.addAction(nt);
    nt = new QAction(QIcon(":/16/folder-tar"),tr("Export pages to directory"),nullptr);
    connect(nt,SIGNAL(triggered()),this,SLOT(exportPagesCtx()));
    cm.addAction(nt);
    cm.addSeparator();
    nt = new QAction(QIcon(":/16/go-down"),tr("Minimize window"),nullptr);
    connect(nt,SIGNAL(triggered()),this,SLOT(minimizeWindowCtx()));
    cm.addAction(nt);
    cm.exec(event->globalPos());
    event->accept();
}

void ZMangaView::redrawPage()
{
    redrawPageEx(QImage(),currentPage);
}

void ZMangaView::redrawPageEx(const QImage& scaled, int page)
{
    if (!scaled.isNull() && page!=currentPage) return;

    QPalette p = palette();
    p.setBrush(QPalette::Dark,QBrush(zg->backgroundColor));
    setPalette(p);

    if (openedFile.isEmpty()) return;
    if (page<0 || page>=privPageCount) return;

    int scb = 6;
    if (scroller->verticalScrollBar() && scroller->verticalScrollBar()->isVisible()) {
        scb = scroller->verticalScrollBar()->width();
    } else if (scroller->horizontalScrollBar() && scroller->horizontalScrollBar()->isVisible()) {
        scb = scroller->horizontalScrollBar()->height();
    }

    // Draw current page
    curPixmap = QPixmap();

    if (!curUmPixmap.isNull()) {
        QSize scrollerSize(scroller->size().width()-scb*2,scroller->size().height()-scb*2);
        QSize targetSize = curUmPixmap.size();
        curPixmap = curUmPixmap;
        if (!curUmPixmap.isNull() && curUmPixmap.height()>0) {
            double pixAspect = (double)curUmPixmap.width()/(double)curUmPixmap.height();
            double myAspect = (double)width()/(double)height();
            if (zoomAny>0) {
                targetSize = curUmPixmap.size()*((double)(zoomAny)/100.0);
            } else if (zoomMode==Fit) {
                if ( pixAspect > myAspect ) // the image is wider than widget -> resize by width
                    targetSize = QSize(scrollerSize.width(),
                                       round(((double)scrollerSize.width())/pixAspect));
                else
                    targetSize = QSize(round(((double)scrollerSize.height())*pixAspect),
                                       scrollerSize.height());
            } else if (zoomMode==Width) {
                targetSize = QSize(scrollerSize.width(),
                                   round(((double)scrollerSize.width())/pixAspect));
            } else if (zoomMode==Height) {
                targetSize = QSize(round(((double)scrollerSize.height())*pixAspect),
                                   scrollerSize.height());
            }

            if (targetSize!=curUmPixmap.size()) {
                if (!scaled.isNull()) {
                    curPixmap = QPixmap::fromImage(scaled);
                } else {
                    // fast inplace resampling
                    curPixmap = curUmPixmap.scaled(targetSize,Qt::IgnoreAspectRatio,Qt::FastTransformation);

                    if (zg->useFineRendering) {
                        Blitz::ScaleFilterType filter;
                        if (targetSize.width()>curUmPixmap.width())
                            filter = zg->upscaleFilter;
                        else
                            filter = zg->downscaleFilter;

                        if (filter!=Blitz::UndefinedFilter) {
                            QImage image = curUmPixmap.toImage();
                            QtConcurrent::run(&resamplersPool,[this,targetSize,filter,image,page](){
                                QImage res = resizeImage(image,targetSize,true,filter,page,&currentPage);

                                if (!res.isNull())
                                    QMetaObject::invokeMethod(this,"redrawPageEx",Qt::QueuedConnection,
                                                              Q_ARG(QImage, res),
                                                              Q_ARG(int, page));
                            });
                        }
                    }
                }
            }
        }
        setMinimumSize(curPixmap.width(),curPixmap.height());

        if (targetSize.height()<scrollerSize.height()) targetSize.setHeight(scrollerSize.height());
        if (targetSize.width()<scrollerSize.width()) targetSize.setWidth(scrollerSize.width());
        resize(targetSize);
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

int exportMangaPage(ZMangaView* view, const QDir& dir, int fnlen, const QString& fmt, int quality, int idx)
{
    if (view->exportFileError) return -1;
    QString fname = dir.filePath(QString("%1.%2").arg(idx+1,fnlen,10,QChar('0')).arg(fmt.toLower()));
    ZMangaLoader *zl = new ZMangaLoader();
    view->connect(zl,&ZMangaLoader::gotError,[view](const QString&){
        view->exportFileError = true;
    });
    view->connect(zl,&ZMangaLoader::gotPage,[idx,fname,fmt,quality,view](const QByteArray& page, const int&,
            const QString&, const QUuid&){
        if (view->exportFileError) return;
        if (page.isEmpty()) return;

        QPixmap p = QPixmap();
        if (!p.loadFromData(page))
            p = QPixmap();
        if (p.isNull()) return;
        if (!p.save(fname,fmt.toLatin1(),quality)) {
            view->exportFileError = true;
            return;
        }
    });

    zl->openFile(view->openedFile,0);
    if (!view->exportFileError)
        zl->getPage(idx);
    zl->closeFile();
    delete zl;
    return idx;
}

void ZMangaView::exportPagesCtx()
{
    if (openedFile.isEmpty()) return;
    if (currentPage<0 || currentPage>=privPageCount) return;

    exportDialog.setPages(currentPage,privPageCount-currentPage);
    exportDialog.setParent(window(),Qt::Dialog);
    exportDialog.setExportDir(zg->savedAuxSaveDir);
    if (!exportDialog.exec()) return;

    QString fmt = exportDialog.getImageFormat();
    QDir dir(exportDialog.getExportDir());
    zg->savedAuxSaveDir = exportDialog.getExportDir();

    int cnt = exportDialog.getPagesCount();
    int fnlen = QString::number(cnt).length();
    int quality = exportDialog.getImageQuality();

    QProgressDialog *dlg = new QProgressDialog(this);
    dlg->setWindowModality(Qt::WindowModal);
    dlg->setAutoClose(false);
    dlg->setWindowTitle(tr("Exporting pages..."));
    dlg->show();
    exportFileError = false;

    QList<int> nums;
    for (int i=0;i<cnt;i++)
        nums << (i+currentPage);

    using namespace std::placeholders; // QTBUG-33735 workaround
    auto save_F = std::bind(exportMangaPage, this, dir, fnlen, fmt, quality, _1);
    QFuture<int> saveMap = QtConcurrent::mapped(nums,save_F);

    QFutureWatcher<void> *mapWatcher = new QFutureWatcher<void>();

    connect(mapWatcher,&QFutureWatcher<void>::progressValueChanged,
            dlg,&QProgressDialog::setValue,Qt::QueuedConnection);
    connect(mapWatcher,&QFutureWatcher<void>::progressRangeChanged,
            dlg,&QProgressDialog::setRange,Qt::QueuedConnection);
    connect(dlg,&QProgressDialog::canceled,mapWatcher,&QFutureWatcher<void>::cancel);
    connect(mapWatcher,&QFutureWatcher<void>::progressTextChanged,
            dlg,&QProgressDialog::setLabelText,Qt::QueuedConnection);
    connect(mapWatcher,&QFutureWatcher<void>::finished,[this,dlg,mapWatcher](){
        QString msg = tr("Pages export completed.");
        if (exportFileError)
            msg = tr("Error caught while saving image. Cannot create file. Check your permissions.");
        if (dlg->wasCanceled())
            msg = tr("Pages export aborted.");
        QMessageBox::information(this,tr("QManga"),msg);
        dlg->close();
        dlg->deleteLater();
        mapWatcher->deleteLater();
    });
    mapWatcher->setFuture(saveMap);
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

void ZMangaView::asyncMsg(const QString &msg)
{
    if (msg.isEmpty()) return;
    QMessageBox::warning(this,tr("QManga"),msg);
}

void ZMangaView::loaderMsg(const QString &msg)
{
    emit auxMessage(msg);
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
