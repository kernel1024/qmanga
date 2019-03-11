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
#include <climits>
#include <cmath>
#include "zmangaloader.h"
#include "zmangaview.h"
#include "zglobal.h"
#include "mainwindow.h"

ZMangaView::ZMangaView(QWidget *parent) :
    QWidget(parent)
{
    scroller = nullptr;
    currentPage = 0;
    privPageCount = 0;
    rotation = 0;
    scrollAccumulator = 0;
    zoomMode = zmFit;
    mouseMode = mmOCR;
    zoomDynamic = false;
    zoomPos = QPoint();
    dragPos = QPoint();
    copyPos = QPoint();
    drawPos = QPoint();
    cropPos = QPoint();
    cropRect = QRect();
    copySelection = new QRubberBand(QRubberBand::Rectangle,this);
    copySelection->hide();
    copySelection->setGeometry(QRect());
    cropSelection = new QRubberBand(QRubberBand::Rectangle,this);
    cropSelection->hide();
    cropSelection->setGeometry(QRect());
    curPixmap = QImage();
    curUmPixmap = QImage();
    openedFile = QString();
    zoomAny = -1;
    iCacheImages.clear();
    iCacheData.clear();
    pathCache.clear();
    processingPages.clear();
    cacheLoaders.clear();
    lastSizes.clear();
    lastFileSizes.clear();
    setMouseTracking(true);

    setBackgroundRole(QPalette::Dark);
    QPalette p = palette();
    p.setBrush(QPalette::Dark,QBrush(QColor(Qt::black)));
    setPalette(p);

    connect(this,&ZMangaView::changeMangaCover,zg->db,&ZDB::sqlChangeFilePreview,Qt::QueuedConnection);
    connect(this,&ZMangaView::updateFileStats,zg->db,&ZDB::sqlUpdateFileStats,Qt::QueuedConnection);

    connect(this,&ZMangaView::requestRedrawPageEx,this,&ZMangaView::redrawPageEx,Qt::QueuedConnection);

    int cnt = QThreadPool::globalInstance()->maxThreadCount()+1;
    if (cnt<2) cnt=2;
    cacheLoaders.reserve(cnt);
    for (int i=0;i<cnt;i++) {
        auto th = new QThread();
        auto ld = new ZMangaLoader();

        connect(ld,&ZMangaLoader::gotPage,this,&ZMangaView::cacheGotPage,Qt::QueuedConnection);
        connect(ld,&ZMangaLoader::gotError,this,&ZMangaView::cacheGotError,Qt::QueuedConnection);
        connect(ld,&ZMangaLoader::closeFileRequest,this,&ZMangaView::closeFile,Qt::QueuedConnection);
        connect(ld,&ZMangaLoader::auxMessage,this,&ZMangaView::loaderMsg,Qt::QueuedConnection);

        connect(this,&ZMangaView::cacheOpenFile,ld,&ZMangaLoader::openFile,Qt::QueuedConnection);
        connect(this,&ZMangaView::cacheCloseFile,ld,&ZMangaLoader::closeFile,Qt::QueuedConnection);

        connect(th,&QThread::finished,ld,&QObject::deleteLater,Qt::QueuedConnection);
        connect(th,&QThread::finished,th,&QObject::deleteLater,Qt::QueuedConnection);

        if (i==0) {
            connect(ld,&ZMangaLoader::gotPageCount,this,
                    &ZMangaView::cacheGotPageCount,Qt::QueuedConnection);
        }
        ZLoaderHelper h = ZLoaderHelper(th,ld);
        ld->threadID = h.id;

        cacheLoaders << h;

        ld->moveToThread(th);
        th->start();
    }
}

ZMangaView::~ZMangaView()
{
    for (const auto &i : qAsConst(cacheLoaders))
        i.thread->quit();
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
    bool preferImages = zg->cachePixmaps;

    auto loader = cacheLoaders.at(idx).loader;
    QMetaObject::invokeMethod(loader,[loader,num,preferImages]{
        if (loader)
            loader->getPage(num,preferImages);
    },Qt::QueuedConnection);
}

void ZMangaView::setZoomMode(ZMangaView::ZoomMode mode)
{
    zoomMode = mode;
    redrawPage();
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
    if ((zg->cachePixmaps && iCacheImages.contains(currentPage)) ||
            (!zg->cachePixmaps && iCacheData.contains(currentPage)))
        displayCurrentPage();
    cacheFillNearest();
}

void ZMangaView::setMouseMode(ZMangaView::MouseMode mode)
{
    mouseMode = mode;
}

void ZMangaView::displayCurrentPage()
{
    if ((zg->cachePixmaps && iCacheImages.contains(currentPage)) ||
            (!zg->cachePixmaps && iCacheData.contains(currentPage))) {
        QImage p = QImage();
        qint64 ffsz = 0;
        if (!zg->cachePixmaps) {
            QByteArray img = iCacheData.value(currentPage);
            ffsz = img.size();
            if (!p.loadFromData(img))
                p = QImage();
            img.clear();
        } else {
            p = iCacheImages.value(currentPage);
        }
        if (rotation!=0) {
            QMatrix mr;
            mr.rotate(rotation*90.0);
            p = p.transformed(mr,Qt::SmoothTransformation);
        }
        if (!cropRect.isNull()) {
            p = p.copy(cropRect);
        }

        if (!p.isNull()) {
            if (lastFileSizes.count()>10)
                lastFileSizes.removeFirst();
            if (lastSizes.count()>10)
                lastSizes.removeFirst();

            lastFileSizes << static_cast<int>(ffsz);
            lastSizes << p.size();

            if (lastFileSizes.count()>0 && lastSizes.count()>0) {
                qint64 sum = 0;
                for (const auto &i : qAsConst(lastFileSizes))
                    sum += i;
                sum = sum / lastFileSizes.count();
                qint64 sumx = 0;
                qint64 sumy = 0;
                for (const auto &i : qAsConst(lastSizes)) {
                    sumx += i.width();
                    sumy += i.height();
                }
                sumx = sumx / lastSizes.count();
                sumy = sumy / lastSizes.count();
                emit averageSizes(QSize(static_cast<int>(sumx),
                                        static_cast<int>(sumy)),sum);
            }
        }

        curUmPixmap = p;
        emit loadedPage(currentPage,pathCache.value(currentPage));
        setFocus();
        redrawPage();
    }
}

void ZMangaView::openFile(const QString &filename)
{
    openFileEx(filename,0);
}

void ZMangaView::openFileEx(const QString &filename, int page)
{
    iCacheImages.clear();
    iCacheData.clear();
    pathCache.clear();
    currentPage = 0;
    curUmPixmap = QImage();
    privPageCount = 0;
    cropRect = QRect();
    redrawPage();
    emit loadedPage(-1,QString());

    emit cacheOpenFile(filename,page);
    if (filename.startsWith(QStringLiteral(":CLIP:")))
        openedFile = QStringLiteral("Clipboard image");
    else
        openedFile = filename;
    processingPages.clear();
    emit updateFileStats(filename);
    emit cropUpdated(cropRect);
}

void ZMangaView::closeFile()
{
    emit cacheCloseFile();
    cropRect = QRect();
    curPixmap = QImage();
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
        }

        // at the page border, attempting to flip the page
        sf = zg->scrollFactor;
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
        w.drawImage(x,y,curPixmap);
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
                    QImage zoomed = curUmPixmap.copy(cutBox);
                    baseRect = QRect(QPoint(zoomPos.x()-zoomed.width()/2,zoomPos.y()-zoomed.height()/2),
                                     zoomed.size());
                    if (baseRect.left()<0) baseRect.moveLeft(0);
                    if (baseRect.right()>width()) baseRect.moveRight(width());
                    if (baseRect.top()<0) baseRect.moveTop(0);
                    if (baseRect.bottom()>height()) baseRect.moveBottom(height());
                    w.drawImage(baseRect,zoomed);
                } else {
                    QRect cutBox(mp.x()-msz/4,mp.y()-msz/4,msz/2,msz/2);
                    baseRect = curUmPixmap.rect();
                    if (cutBox.left()<baseRect.left()) cutBox.moveLeft(baseRect.left());
                    if (cutBox.right()>baseRect.right()) cutBox.moveRight(baseRect.right());
                    if (cutBox.top()<baseRect.top()) cutBox.moveTop(baseRect.top());
                    if (cutBox.bottom()>baseRect.bottom()) cutBox.moveBottom(baseRect.bottom());
                    QImage zoomed = curUmPixmap.copy(cutBox);
                    zoomed = resizeImage(zoomed,zoomed.size()*3.0,true,zg->upscaleFilter);
                    baseRect = QRect(QPoint(zoomPos.x()-zoomed.width()/2,zoomPos.y()-zoomed.height()/2),
                                     zoomed.size());
                    if (baseRect.left()<0) baseRect.moveLeft(0);
                    if (baseRect.right()>width()) baseRect.moveRight(width());
                    if (baseRect.top()<0) baseRect.moveTop(0);
                    if (baseRect.bottom()>height()) baseRect.moveBottom(height());
                    w.drawImage(baseRect,zoomed);
                }
            }

        }
    } else {
        if ((iCacheData.contains(currentPage) || iCacheImages.contains(currentPage))
                && curUmPixmap.isNull()) {
            QPixmap p(QStringLiteral(":/32/edit-delete"));
            w.drawPixmap((width()-p.width())/2,(height()-p.height())/2,p);
            w.setPen(QPen(zg->foregroundColor()));
            w.drawText(0,(height()-p.height())/2+p.height()+5,
                       width(),w.fontMetrics().height(),Qt::AlignCenter,
                       tr("Error loading page %1").arg(currentPage+1));
        }
    }
}

void ZMangaView::mouseMoveEvent(QMouseEvent *event)
{
    QScrollBar* vb = scroller->verticalScrollBar();
    QScrollBar* hb = scroller->horizontalScrollBar();
    if ((event->buttons() & Qt::LeftButton)>0) {
        if (mouseMode == mmPan) {
            // drag move in progress
            if (!dragPos.isNull()) {
                int hlen = hb->maximum()-hb->minimum();
                int vlen = vb->maximum()-vb->minimum();
                int hvsz = width();
                int vvsz = height();
                hb->setValue(hb->value()+hlen*(dragPos.x()-event->pos().x())/hvsz);
                vb->setValue(vb->value()+vlen*(dragPos.y()-event->pos().y())/vvsz);
            }
        } else if (mouseMode == mmCrop && cropRect.isNull()) {
            // crop move in progress
            cropSelection->setGeometry(QRect(event->pos(),cropPos).normalized());
        } else if (mouseMode == mmOCR){
#ifdef WITH_OCR
            // OCR move in progress
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
        if (mouseMode == mmOCR) {
#ifdef WITH_OCR
            // start OCR move
            copyPos = event->pos();
            copySelection->setGeometry(copyPos.x(),copyPos.y(),0,0);
            copySelection->show();
#endif
        } else if (mouseMode == mmCrop && cropRect.isNull()) {
            // start crop move
            cropPos = event->pos();
            cropSelection->setGeometry(cropPos.x(),cropPos.y(),0,0);
            cropSelection->show();
        } else if (mouseMode == mmPan) {
            // start drag move
            dragPos = event->pos();
        }
    }
}

void ZMangaView::mouseDoubleClickEvent(QMouseEvent *)
{
    copyPos = QPoint();
    cropPos = QPoint();
    emit doubleClicked();
}

void ZMangaView::mouseReleaseEvent(QMouseEvent *event)
{
#ifdef WITH_OCR
    if (event->button()==Qt::LeftButton &&
            !copyPos.isNull() &&
            mouseMode == mmOCR) {
        // OCR move completion
        if (!curPixmap.isNull()) {
            QRect cp = QRect(event->pos(),copyPos).normalized();
            cp.moveTo(cp.x()-drawPos.x(),cp.y()-drawPos.y());
            cp.moveTo(cp.left()*curUmPixmap.width()/curPixmap.width(),
                      cp.top()*curUmPixmap.height()/curPixmap.height());
            cp.setWidth(cp.width()*curUmPixmap.width()/curPixmap.width());
            cp.setHeight(cp.height()*curUmPixmap.height()/curPixmap.height());
            cp = cp.intersected(curUmPixmap.rect());
            if (ocr!=nullptr && cp.width()>20 && cp.height()>20) {
                QImage cpx = curUmPixmap.copy(cp);
                ocr->SetImage(Image2PIX(cpx));
                char* rtext = ocr->GetUTF8Text();
                QString s = QString::fromUtf8(rtext);
                delete[] rtext;
                QStringList sl = s.split('\n',QString::SkipEmptyParts);
                int maxlen = 0;
                for (const auto &i : sl)
                    if (i.length()>maxlen)
                        maxlen = i.length();
                if (maxlen<sl.count()) { // vertical kanji block, needs transpose
                    QStringList sl2;
                    sl2.reserve(maxlen);
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
    } else if (event->button()==Qt::LeftButton &&
               !cropPos.isNull() &&
               cropRect.isNull() &&
               mouseMode == mmCrop) {
        // crop move completion
        if (!curPixmap.isNull()) {
            QRect cp = QRect(event->pos(),cropPos).normalized();
            cp.moveTo(cp.x()-drawPos.x(),cp.y()-drawPos.y());
            cp.moveTo(cp.left()*curUmPixmap.width()/curPixmap.width(),
                      cp.top()*curUmPixmap.height()/curPixmap.height());
            cp.setWidth(cp.width()*curUmPixmap.width()/curPixmap.width());
            cp.setHeight(cp.height()*curUmPixmap.height()/curPixmap.height());
            cp = cp.intersected(curUmPixmap.rect());

            cropRect = cp;
            displayCurrentPage();
            emit cropUpdated(cropRect);
        }
    }
#else
    Q_UNUSED(event)
#endif
    dragPos = QPoint();
    copyPos = QPoint();
    cropPos = QPoint();
    copySelection->hide();
    copySelection->setGeometry(QRect());
    cropSelection->hide();
    cropSelection->setGeometry(QRect());
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
    connect(nt,&QAction::triggered,this,&ZMangaView::setZoomFit);
    cm.addAction(nt);
    nt = new QAction(QIcon(":/16/zoom-fit-width"),tr("Zoom width"),nullptr);
    connect(nt,&QAction::triggered,this,&ZMangaView::setZoomWidth);
    cm.addAction(nt);
    nt = new QAction(QIcon(":/16/zoom-fit-height"),tr("Zoom height"),nullptr);
    connect(nt,&QAction::triggered,this,&ZMangaView::setZoomHeight);
    cm.addAction(nt);
    nt = new QAction(QIcon(":/16/zoom-original"),tr("Zoom original"),nullptr);
    connect(nt,&QAction::triggered,this,&ZMangaView::setZoomOriginal);
    cm.addAction(nt);
    cm.addSeparator();
    nt = new QAction(QIcon(":/16/zoom-draw"),tr("Zoom dynamic"),nullptr);
    nt->setCheckable(true);
    nt->setChecked(zoomDynamic);
    connect(nt,&QAction::triggered,this,&ZMangaView::setZoomDynamic);
    cm.addAction(nt);
    cm.addSeparator();
    nt = new QAction(QIcon(":/16/view-refresh"),tr("Redraw page"),nullptr);
    connect(nt,&QAction::triggered,this,&ZMangaView::redrawPage);
    cm.addAction(nt);
    if (!cropRect.isNull()) {
        nt = new QAction(QIcon(":/16/transform-crop"),tr("Remove page crop"),nullptr);
        connect(nt,&QAction::triggered,this,[this](){
            cropRect = QRect();
            displayCurrentPage();
            emit cropUpdated(cropRect);
        });
        cm.addAction(nt);
    }
    cm.addSeparator();
    nt = new QAction(QIcon(":/16/document-close"),tr("Close manga"),nullptr);
    connect(nt,&QAction::triggered,this,&ZMangaView::closeFileCtx);
    cm.addAction(nt);
    nt = new QAction(QIcon(":/16/view-preview"),tr("Set page as cover"),nullptr);
    connect(nt,&QAction::triggered,this,&ZMangaView::changeMangaCoverCtx);
    cm.addAction(nt);
    nt = new QAction(QIcon(":/16/folder-tar"),tr("Export pages to directory"),nullptr);
    connect(nt,&QAction::triggered,this,&ZMangaView::exportPagesCtx);
    cm.addAction(nt);
    cm.addSeparator();
    nt = new QAction(QIcon(":/16/go-down"),tr("Minimize window"),nullptr);
    connect(nt,&QAction::triggered,this,&ZMangaView::minimizeWindowCtx);
    cm.addAction(nt);

    auto mwnd = qobject_cast<MainWindow *>(window());
    if (mwnd) {
        nt = new QAction(QIcon(":/16/transform-move"),tr("Show fast scroller"),nullptr);
        nt->setCheckable(true);
        nt->setChecked(mwnd->fastScrollPanel->isVisible());
        connect(nt,&QAction::toggled,mwnd->fastScrollPanel,&QFrame::setVisible);
        cm.addAction(nt);
    }
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
    emit backgroundUpdated(zg->backgroundColor);

    if (openedFile.isEmpty()) return;
    if (page<0 || page>=privPageCount) return;

    int scb = 6;
    if (scroller->verticalScrollBar() && scroller->verticalScrollBar()->isVisible()) {
        scb = scroller->verticalScrollBar()->width();
    } else if (scroller->horizontalScrollBar() && scroller->horizontalScrollBar()->isVisible()) {
        scb = scroller->horizontalScrollBar()->height();
    }

    // Draw current page
    curPixmap = QImage();

    if (!curUmPixmap.isNull()) {
        QSize scrollerSize(scroller->size().width()-scb*2,scroller->size().height()-scb*2);
        QSize targetSize = curUmPixmap.size();
        curPixmap = curUmPixmap;
        if (!curUmPixmap.isNull() && curUmPixmap.height()>0) {
            double pixAspect = static_cast<double>(curUmPixmap.width()) /
                               static_cast<double>(curUmPixmap.height());
            double myAspect = static_cast<double>(width()) /
                              static_cast<double>(height());
            if (zoomAny>0) {
                targetSize = curUmPixmap.size()*(static_cast<double>(zoomAny)/100.0);
            } else if (zoomMode==zmFit) {
                if ( pixAspect > myAspect ) // the image is wider than widget -> resize by width
                    targetSize = QSize(scrollerSize.width(),
                                       qRound((static_cast<double>(scrollerSize.width()))/pixAspect));
                else
                    targetSize = QSize(qRound((static_cast<double>(scrollerSize.height()))*pixAspect),
                                       scrollerSize.height());
            } else if (zoomMode==zmWidth) {
                targetSize = QSize(scrollerSize.width(),
                                   qRound((static_cast<double>(scrollerSize.width()))/pixAspect));
            } else if (zoomMode==zmHeight) {
                targetSize = QSize(qRound((static_cast<double>(scrollerSize.height()))*pixAspect),
                                   scrollerSize.height());
            }

            if (targetSize!=curUmPixmap.size()) {
                if (!scaled.isNull()) {
                    curPixmap = scaled;
                } else {
                    // fast inplace resampling
                    curPixmap = curUmPixmap.scaled(targetSize,Qt::IgnoreAspectRatio,
                                                   Qt::FastTransformation);

                    if (zg->useFineRendering) {
                        Blitz::ScaleFilterType filter;
                        if (targetSize.width()>curUmPixmap.width())
                            filter = zg->upscaleFilter;
                        else
                            filter = zg->downscaleFilter;

                        if (filter!=Blitz::UndefinedFilter) {
                            QImage image = curUmPixmap;
                            QtConcurrent::run(&resamplersPool,[this,targetSize,filter,image,page](){
                                QImage res = resizeImage(image,targetSize,true,filter,page,&currentPage);

                                if (!res.isNull())
                                    emit requestRedrawPageEx(res, page);
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

static QAtomicInt exportFileError;

ZExportWork ZMangaView::exportMangaPage(const ZExportWork& item)
{
    if (exportFileError>0 || !item.isValid()) return ZExportWork();

    int quality = item.quality;
    const QString format = item.format;
    QString fname = item.dir.filePath(QStringLiteral("%1.%2")
                                         .arg(item.idx+1,item.filenameLength,10,QChar('0'))
                                         .arg(format.toLower()));
    auto zl = new ZMangaLoader();
    connect(zl,&ZMangaLoader::gotError,[](const QString&){
        exportFileError++;
    });

    connect(zl,&ZMangaLoader::gotPage,[fname,quality,format]
            (const QByteArray& page, const QImage& image, const int&, const QString&, const QUuid&){
        Q_UNUSED(page)
        if (exportFileError>0) return;
        if (image.isNull()) return;
        if (!image.save(fname,format.toLatin1(),quality)) {
            exportFileError++;
            return;
        }
    });

    zl->openFile(item.sourceFile,0);
    if (!exportFileError)
        zl->getPage(item.idx,true);
    zl->closeFile();
    delete zl;
    return ZExportWork();
}

void ZMangaView::exportPagesCtx()
{
    if (openedFile.isEmpty()) return;
    if (currentPage<0 || currentPage>=privPageCount) return;

    exportDialog.setPages(currentPage,privPageCount-currentPage);
    exportDialog.setParent(window(),Qt::Dialog);
    exportDialog.setExportDir(zg->savedAuxSaveDir);
    if (!exportDialog.exec()) return;

    zg->savedAuxSaveDir = exportDialog.getExportDir();

    int cnt = exportDialog.getPagesCount();

    QProgressDialog *dlg = new QProgressDialog(this);
    dlg->setWindowModality(Qt::WindowModal);
    dlg->setAutoClose(false);
    dlg->setWindowTitle(tr("Exporting pages..."));
    dlg->show();
    exportFileError = 0;

    QVector<ZExportWork> nums;
    nums.reserve(cnt);
    for (int i=0;i<cnt;i++)
        nums << ZExportWork(i+currentPage,QDir(exportDialog.getExportDir()),
                            openedFile,exportDialog.getImageFormat(),
                            QString::number(cnt).length(),
                            exportDialog.getImageQuality());

    QFuture<ZExportWork> saveMap = QtConcurrent::mapped(nums,exportMangaPage);

    auto mapWatcher = new QFutureWatcher<void>();

    connect(mapWatcher,&QFutureWatcher<void>::progressValueChanged,
            dlg,&QProgressDialog::setValue,Qt::QueuedConnection);
    connect(mapWatcher,&QFutureWatcher<void>::progressRangeChanged,
            dlg,&QProgressDialog::setRange,Qt::QueuedConnection);
    connect(dlg,&QProgressDialog::canceled,mapWatcher,&QFutureWatcher<void>::cancel);
    connect(mapWatcher,&QFutureWatcher<void>::progressTextChanged,
            dlg,&QProgressDialog::setLabelText,Qt::QueuedConnection);
    connect(mapWatcher,&QFutureWatcher<void>::finished,this,[this,dlg,mapWatcher](){
        QString msg = tr("Pages export completed.");
        if (exportFileError>0)
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
    setZoomMode(zmFit);
}

void ZMangaView::setZoomWidth()
{
    setZoomMode(zmWidth);
}

void ZMangaView::setZoomHeight()
{
    setZoomMode(zmHeight);
}

void ZMangaView::setZoomOriginal()
{
    setZoomMode(zmOriginal);
}

void ZMangaView::setZoomDynamic(bool state)
{
    zoomDynamic = state;
    redrawPage();
}

void ZMangaView::setZoomAny(const QString &proc)
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

void ZMangaView::loaderMsg(const QString &msg)
{
    emit auxMessage(msg);
}

void ZMangaView::cacheGotPage(const QByteArray &page, const QImage &pageImage, const int &num,
                              const QString &internalPath, const QUuid &threadID)
{
    if (cacheLoaders.contains(ZLoaderHelper(threadID))) {
        cacheLoaders[cacheLoaders.indexOf(ZLoaderHelper(threadID))].delJob();
    }
    if (processingPages.contains(num))
        processingPages.removeOne(num);

    if ((zg->cachePixmaps && iCacheImages.contains(num)) ||
            (!zg->cachePixmaps && iCacheData.contains(num))) return;

    if (!zg->cachePixmaps)
        iCacheData[num]=page;
    else
        iCacheImages[num]=pageImage;
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
        iCacheImages.clear();

    QIntVector toCache = cacheGetActivePages();
    QList<int> cached;
    if (zg->cachePixmaps)
        cached = iCacheImages.keys();
    else
        cached = iCacheData.keys();
    for (const auto &i : qAsConst(cached)) {
        if (!toCache.contains(i)) {
            if (zg->cachePixmaps)
                iCacheImages.remove(i);
            else
                iCacheData.remove(i);
            pathCache.remove(i);
        }
    }
}

void ZMangaView::cacheFillNearest()
{
    QIntVector toCache = cacheGetActivePages();
    int idx = 0;
    while (idx<toCache.count()) {
        bool contains = false;
        if (zg->cachePixmaps)
            contains = iCacheImages.contains(toCache.at(idx));
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

QIntVector ZMangaView::cacheGetActivePages()
{
    QIntVector l;
    l.reserve(zg->cacheWidth*2+1);

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

    l << currentPage; // load current page at first

    for (int i=0;i<cacheRadius;i++) {

        if (!l.contains(i))
            l << i; // first pages

        if (!l.contains(privPageCount-i-1))
            l << privPageCount-i-1;  // last pages
    }
    for (int i=(currentPage-cacheRadius);i<(currentPage+cacheRadius);i++) {
        if (i>=0 && i<privPageCount && !l.contains(i)) // pages around current page
            l << i;
    }
    return l;
}
