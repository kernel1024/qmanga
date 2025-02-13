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
#include <QThreadPool>
#include <QElapsedTimer>
#include <QComboBox>
#include <utility>
#include <algorithm>
#include <execution>
#include <functional>
#include <climits>
#include <cmath>
#include "zmangaloader.h"
#include "zmangaview.h"
#include "zglobal.h"
#include "mainwindow.h"
#include "ocr/abstractocr.h"

ZMangaView::ZMangaView(QWidget *parent) :
    QWidget(parent)
{
    m_copySelection = new QRubberBand(QRubberBand::Rectangle,this);
    m_copySelection->hide();
    m_copySelection->setGeometry(QRect());
    m_cropSelection = new QRubberBand(QRubberBand::Rectangle,this);
    m_cropSelection->hide();
    m_cropSelection->setGeometry(QRect());

    setMouseTracking(true);

    setBackgroundRole(QPalette::Dark);
    QPalette p = palette();
    p.setBrush(QPalette::Dark,QBrush(QColor(Qt::black)));
    setPalette(p);

    connect(this,&ZMangaView::changeMangaCover,zF->global()->db(),&ZDB::sqlChangeFilePreview,Qt::QueuedConnection);
    connect(this,&ZMangaView::updateFileStats,zF->global()->db(),&ZDB::sqlUpdateFileStats,Qt::QueuedConnection);

    connect(this,&ZMangaView::requestRedrawPageEx,this,&ZMangaView::redrawPageEx,Qt::QueuedConnection);

    connect(this,&ZMangaView::addOCRText,zF->global()->ocrEditor(),&ZOCREditor::addOCRText);

    int cnt = QThreadPool::globalInstance()->maxThreadCount()+1;
    if (cnt<2) cnt=2;
    m_cacheLoaders.reserve(cnt);
    for (int i=0;i<cnt;i++) {
        auto *th = new QThread();
        auto *ld = new ZMangaLoader();

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

        m_cacheLoaders << h;

        ld->moveToThread(th);
        th->setObjectName(QSL("MLoader#%1").arg(i));
        th->start();
    }
}

ZMangaView::~ZMangaView()
{
    for (const auto &i : std::as_const(m_cacheLoaders))
        i.thread->quit();
    m_cacheLoaders.clear();
}

void ZMangaView::cacheGetPage(int num)
{
    int idx = 0;
    int jcnt = INT_MAX;
    for (int i=0;i<m_cacheLoaders.count();i++) {
        if (m_cacheLoaders.at(i).jobCount<jcnt) {
            idx=i;
            jcnt=m_cacheLoaders.at(i).jobCount;
        }
    }

    m_cacheLoaders[idx].addJob();
    bool preferImages = zF->global()->getCachePixmaps();

    auto loader = m_cacheLoaders.at(idx).loader;
    QMetaObject::invokeMethod(loader,[loader,num,preferImages]{
        if (loader)
            loader->getPage(num,preferImages);
    },Qt::QueuedConnection);
}

void ZMangaView::setZoomMode(int mode)
{
    m_zoomMode = static_cast<ZMangaView::ZoomMode>(mode);
    redrawPage();
}

int ZMangaView::getPageCount() const
{
    return m_privPageCount;
}

void ZMangaView::getPage(int num)
{
    if (!m_scroller) return;
    m_currentPage = num;

    if (m_scroller->verticalScrollBar()->isVisible())
        m_scroller->verticalScrollBar()->setValue(0);
    if (m_scroller->horizontalScrollBar()->isVisible())
        m_scroller->horizontalScrollBar()->setValue(0);

    cacheDropUnusable();
    if (m_iCacheImages.contains(m_currentPage) || m_iCacheData.contains(m_currentPage))
        displayCurrentPage();
    cacheFillNearest();
}

void ZMangaView::setMouseMode(ZMangaView::MouseMode mode)
{
    m_mouseMode = mode;
}

QString ZMangaView::getOpenedFile() const
{
    return m_openedFile;
}

int ZMangaView::getCurrentPage() const
{
    return m_currentPage;
}

void ZMangaView::setScroller(QScrollArea *scroller)
{
    m_scroller = scroller;
}

void ZMangaView::displayCurrentPage()
{
    QImage p;
    qint64 ffsz = 0;

    if (m_iCacheImages.contains(m_currentPage)) {
        p = m_iCacheImages.value(m_currentPage);

    } else if (m_iCacheData.contains(m_currentPage)) {
        QByteArray img = m_iCacheData.value(m_currentPage);
        ffsz = img.size();
        if (!p.loadFromData(img))
            p = QImage();
        img.clear();

    } else {
        return;
    }

    if (m_rotation!=0) {
        QTransform mr;
        mr.rotateRadians(m_rotation*M_PI_2);
        p = p.transformed(mr,Qt::SmoothTransformation);
    }
    if (!m_cropRect.isNull()) {
        p = p.copy(m_cropRect);
    }

    if (!p.isNull()) {
        if (m_lastFileSizes.count()>ZDefaults::avgSizeSamplesCount)
            m_lastFileSizes.removeFirst();
        if (m_lastSizes.count()>ZDefaults::avgSizeSamplesCount)
            m_lastSizes.removeFirst();

        m_lastFileSizes << static_cast<int>(ffsz);
        m_lastSizes << p.size();

        if (m_lastFileSizes.count()>0 && m_lastSizes.count()>0) {
            qint64 sum = 0;
            for (const auto &i : std::as_const(m_lastFileSizes))
                sum += i;
            sum = sum / m_lastFileSizes.count();
            qint64 sumx = 0;
            qint64 sumy = 0;
            for (const auto &i : std::as_const(m_lastSizes)) {
                sumx += i.width();
                sumy += i.height();
            }
            sumx = sumx / m_lastSizes.count();
            sumy = sumy / m_lastSizes.count();
            Q_EMIT averageSizes(QSize(static_cast<int>(sumx),
                                      static_cast<int>(sumy)),sum);
        }
    }

    m_curUnscaledPixmap = p;
    Q_EMIT loadedPage(m_currentPage,m_pathCache.value(m_currentPage));
    setFocus();
    redrawPage();
}

void ZMangaView::openFile(const QString &filename)
{
    openFileEx(filename,0);
}

void ZMangaView::openFileEx(const QString &filename, int page)
{
    m_iCacheImages.clear();
    m_iCacheData.clear();
    m_pathCache.clear();
    m_currentPage = 0;
    m_curUnscaledPixmap = QImage();
    m_privPageCount = 0;
    m_cropRect = QRect();
    redrawPage();
    Q_EMIT loadedPage(-1,QString());

    Q_EMIT cacheOpenFile(filename,page);
    if (filename.startsWith(QSL(":CLIP:"))) {
        m_openedFile = QSL("Clipboard image");
    } else {
        m_openedFile = filename;
    }
    m_processingPages.clear();
    Q_EMIT updateFileStats(filename);
    Q_EMIT cropUpdated(m_cropRect);
}

void ZMangaView::closeFile()
{
    Q_EMIT cacheCloseFile();
    m_cropRect = QRect();
    m_curPixmap = QImage();
    m_openedFile = QString();
    m_privPageCount = 0;
    update();
    Q_EMIT loadedPage(-1,QString());
    m_processingPages.clear();
}

void ZMangaView::setPage(int page)
{
    m_scrollAccumulator = 0;
    if (page<0 || page>=m_privPageCount) return;
    getPage(page);
}

void ZMangaView::wheelEvent(QWheelEvent *event)
{
    if (!m_scroller) return;
    int sf = 1;
    int dy = event->angleDelta().y();
    QScrollBar *vb = m_scroller->verticalScrollBar();
    if (vb!=nullptr && vb->isVisible()) { // page is zoomed and scrollable
        if (((vb->value()>vb->minimum()) && (vb->value()<vb->maximum()))    // somewhere middle of page
                || ((vb->value()==vb->minimum()) && (dy<0))                 // at top, scrolling down
                || ((vb->value()==vb->maximum()) && (dy>0))) {              // at bottom, scrolling up
            m_scrollAccumulator = 0;
            event->ignore();
            return;
        }

        // at the page border, attempting to flip the page
        sf = zF->global()->getScrollFactor();
    }

    if (abs(dy)<zF->global()->getDetectedDelta())
        zF->global()->setDetectedDelta(abs(dy));

    m_scrollAccumulator+= dy;
    int numSteps = m_scrollAccumulator / (zF->global()->getScrollDelta() * sf);

    if (numSteps!=0)
        setPage(m_currentPage-numSteps);

    event->accept();
}

void ZMangaView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    if (!m_scroller) return;

    QPainter w(this);
    if (!m_curPixmap.isNull()) {
        int x=0;
        int y=0;
        if (m_curPixmap.width() < m_scroller->viewport()->width())
            x = (m_scroller->viewport()->width() - m_curPixmap.width()) / 2;
        if (m_curPixmap.height() < m_scroller->viewport()->height())
            y = (m_scroller->viewport()->height() - m_curPixmap.height()) / 2;
        w.drawImage(x,y,m_curPixmap);
        m_drawPos = QPoint(x,y);

        if (m_zoomDynamic) {
            QPoint mp(m_zoomPos.x()-x,m_zoomPos.y()-y);
            QRect baseRect = m_curPixmap.rect();
            if (baseRect.contains(mp,true)) {
                mp.setX(mp.x()*m_curUnscaledPixmap.width()/m_curPixmap.width());
                mp.setY(mp.y()*m_curUnscaledPixmap.height()/m_curPixmap.height());
                int msz = zF->global()->getMagnifySize();

                if (m_curPixmap.width()<m_curUnscaledPixmap.width() || m_curPixmap.height()<m_curUnscaledPixmap.height()) {
                    QRect cutBox(mp.x()-msz/2,mp.y()-msz/2,msz,msz);
                    baseRect = m_curUnscaledPixmap.rect();
                    if (cutBox.left()<baseRect.left()) cutBox.moveLeft(baseRect.left());
                    if (cutBox.right()>baseRect.right()) cutBox.moveRight(baseRect.right());
                    if (cutBox.top()<baseRect.top()) cutBox.moveTop(baseRect.top());
                    if (cutBox.bottom()>baseRect.bottom()) cutBox.moveBottom(baseRect.bottom());
                    QImage zoomed = m_curUnscaledPixmap.copy(cutBox);
                    baseRect = QRect(QPoint(m_zoomPos.x()-zoomed.width()/2,m_zoomPos.y()-zoomed.height()/2),
                                     zoomed.size());
                    if (baseRect.left()<0) baseRect.moveLeft(0);
                    if (baseRect.right()>width()) baseRect.moveRight(width());
                    if (baseRect.top()<0) baseRect.moveTop(0);
                    if (baseRect.bottom()>height()) baseRect.moveBottom(height());
                    w.drawImage(baseRect,zoomed);
                } else {
                    QRect cutBox(mp.x()-msz/4,mp.y()-msz/4,msz/2,msz/2);
                    baseRect = m_curUnscaledPixmap.rect();
                    if (cutBox.left()<baseRect.left()) cutBox.moveLeft(baseRect.left());
                    if (cutBox.right()>baseRect.right()) cutBox.moveRight(baseRect.right());
                    if (cutBox.top()<baseRect.top()) cutBox.moveTop(baseRect.top());
                    if (cutBox.bottom()>baseRect.bottom()) cutBox.moveBottom(baseRect.bottom());
                    QImage zoomed = m_curUnscaledPixmap.copy(cutBox);
                    zoomed = ZGenericFuncs::resizeImage(zoomed,zoomed.size()*ZDefaults::dynamicZoomUpScale,
                                                        true,zF->global()->getUpscaleFilter());
                    baseRect = QRect(QPoint(m_zoomPos.x()-zoomed.width()/2,m_zoomPos.y()-zoomed.height()/2),
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
        if ((m_iCacheData.contains(m_currentPage) || m_iCacheImages.contains(m_currentPage))
                && m_curUnscaledPixmap.isNull()) {
            QPixmap p(QSL(":/32/edit-delete"));
            w.drawPixmap((width()-p.width())/2,(height()-p.height())/2,p);
            w.setPen(QPen(zF->global()->getForegroundColor()));
            w.drawText(0,(height()-p.height())/2+p.height()+ZDefaults::errorPageLoadMsgVerticalMargin,
                       width(),w.fontMetrics().height(),Qt::AlignCenter,
                       tr("Error loading page %1").arg(m_currentPage+1));
        }
    }
}

void ZMangaView::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_scroller) return;
    QScrollBar* vb = m_scroller->verticalScrollBar();
    QScrollBar* hb = m_scroller->horizontalScrollBar();
    if ((event->buttons() & Qt::LeftButton)>0) {
        if (m_mouseMode == mmPan) {
            // drag move in progress
            if (!m_dragPos.isNull()) {
                int hlen = hb->maximum()-hb->minimum();
                int vlen = vb->maximum()-vb->minimum();
                int hvsz = width();
                int vvsz = height();
                hb->setValue(hb->value()+hlen*(m_dragPos.x()-event->pos().x())/hvsz);
                vb->setValue(vb->value()+vlen*(m_dragPos.y()-event->pos().y())/vvsz);
            }
        } else if (m_mouseMode == mmCrop && m_cropRect.isNull()) {
            // crop move in progress
            m_cropSelection->setGeometry(QRect(event->pos(),m_cropPos).normalized());
        } else if (m_mouseMode == mmOCR){
            // OCR move in progress
            m_copySelection->setGeometry(QRect(event->pos(),m_copyPos).normalized());
        }
    } else {
        if ((QApplication::keyboardModifiers() & Qt::ControlModifier) == 0) {
            if (m_zoomDynamic) {
                m_zoomPos = event->pos();
                update();
            } else
                m_zoomPos = QPoint();
        }
    }
}

void ZMangaView::mousePressEvent(QMouseEvent *event)
{
    if (event->button()==Qt::MiddleButton) {
        Q_EMIT minimizeRequested();
        event->accept();
    } else if (event->button()==Qt::LeftButton) {
        if (m_mouseMode == mmOCR) {
            // start OCR move
            m_copyPos = event->pos();
            m_copySelection->setGeometry(m_copyPos.x(),m_copyPos.y(),0,0);
            m_copySelection->show();
        } else if (m_mouseMode == mmCrop && m_cropRect.isNull()) {
            // start crop move
            m_cropPos = event->pos();
            m_cropSelection->setGeometry(m_cropPos.x(),m_cropPos.y(),0,0);
            m_cropSelection->show();
        } else if (m_mouseMode == mmPan) {
            // start drag move
            m_dragPos = event->pos();
        }
    }
}

void ZMangaView::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event)

    m_copyPos = QPoint();
    m_cropPos = QPoint();
    Q_EMIT doubleClicked();
}

void ZMangaView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button()==Qt::LeftButton &&
            !m_copyPos.isNull() &&
            m_mouseMode == mmOCR) {
        // OCR move completion
        if (!m_curPixmap.isNull()) {
            QRect cp = QRect(event->pos(),m_copyPos).normalized();
            cp.moveTo(cp.x()-m_drawPos.x(),cp.y()-m_drawPos.y());
            cp.moveTo(cp.left()*m_curUnscaledPixmap.width()/m_curPixmap.width(),
                      cp.top()*m_curUnscaledPixmap.height()/m_curPixmap.height());
            cp.setWidth(cp.width()*m_curUnscaledPixmap.width()/m_curPixmap.width());
            cp.setHeight(cp.height()*m_curUnscaledPixmap.height()/m_curPixmap.height());
            cp = cp.intersected(m_curUnscaledPixmap.rect());
            if (cp.width() > ZDefaults::ocrSquareMinimumSize
                && cp.height() > ZDefaults::ocrSquareMinimumSize) {
                handleOCRRequest(m_curUnscaledPixmap.copy(cp));
            }
        }
    } else if (event->button()==Qt::LeftButton &&
               !m_cropPos.isNull() &&
               m_cropRect.isNull() &&
               m_mouseMode == mmCrop) {
        // crop move completion
        if (!m_curPixmap.isNull()) {
            QRect cp = QRect(event->pos(),m_cropPos).normalized();
            cp.moveTo(cp.x()-m_drawPos.x(),cp.y()-m_drawPos.y());
            cp.moveTo(cp.left()*m_curUnscaledPixmap.width()/m_curPixmap.width(),
                      cp.top()*m_curUnscaledPixmap.height()/m_curPixmap.height());
            cp.setWidth(cp.width()*m_curUnscaledPixmap.width()/m_curPixmap.width());
            cp.setHeight(cp.height()*m_curUnscaledPixmap.height()/m_curPixmap.height());
            cp = cp.intersected(m_curUnscaledPixmap.rect());

            m_cropRect = cp;
            displayCurrentPage();
            Q_EMIT cropUpdated(m_cropRect);
        }
    }
    m_dragPos = QPoint();
    m_copyPos = QPoint();
    m_cropPos = QPoint();
    m_copySelection->hide();
    m_copySelection->setGeometry(QRect());
    m_cropSelection->hide();
    m_cropSelection->setGeometry(QRect());
}

void ZMangaView::handleOCRRequest(const QImage &image)
{
    auto *ocr = zF->ocr(this);
    if ((ocr == nullptr) || (!(ocr->isReady()))) {
        QMessageBox::critical(
            nullptr,
            QGuiApplication::applicationDisplayName(),
            tr("Could not initialize any OCR backend. \n"
               "Maybe language training data is not installed, or the credentials "
               "for onlince serivces is invalid."));
        return;
    }

    if (ocr != m_previousOCR) {
        connect(ocr, &ZAbstractOCR::gotOCRResult, this, &ZMangaView::ocrFinished);
        m_previousOCR = ocr;
    }

    ocr->processImage(image);
}

void ZMangaView::ocrFinished(const QString& text)
{
    Q_EMIT addOCRText(text.split(QChar(u'\n'),Qt::SkipEmptyParts));
}

void ZMangaView::keyPressEvent(QKeyEvent *event)
{
    QScrollBar* vb = m_scroller->verticalScrollBar();
    QScrollBar* hb = m_scroller->horizontalScrollBar();
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
            Q_EMIT minimizeRequested();
            break;
        default:
            Q_EMIT keyPressed(event->key());
            break;
    }
    event->accept();
}

void ZMangaView::contextMenuEvent(QContextMenuEvent *event)
{
    auto *mwnd = qobject_cast<ZMainWindow *>(window());
    if (mwnd==nullptr) return;

    QMenu cm(this);

    mwnd->addContextMenuItems(&cm);

    cm.addSeparator();
    auto *nt = new QAction(QIcon(QSL(":/16/view-refresh")),tr("Redraw page"),nullptr);
    connect(nt,&QAction::triggered,this,&ZMangaView::redrawPage);
    cm.addAction(nt);
    if (!m_cropRect.isNull()) {
        nt = new QAction(QIcon(QSL(":/16/transform-crop")),tr("Remove page crop"),nullptr);
        connect(nt,&QAction::triggered,this,[this](){
            m_cropRect = QRect();
            displayCurrentPage();
            Q_EMIT cropUpdated(m_cropRect);
        });
        cm.addAction(nt);
    }
    nt = new QAction(QIcon(QSL(":/16/view-preview")),tr("Set page as cover"),nullptr);
    connect(nt,&QAction::triggered,this,&ZMangaView::changeMangaCoverCtx);
    cm.addAction(nt);
    nt = new QAction(QIcon(QSL(":/16/folder-tar")),tr("Export pages to directory"),nullptr);
    connect(nt,&QAction::triggered,this,&ZMangaView::exportPagesCtx);
    cm.addAction(nt);

    cm.exec(event->globalPos());
    event->accept();
}

void ZMangaView::redrawPage()
{
    redrawPageEx(QImage(),m_currentPage);
}

void ZMangaView::redrawPageEx(const QImage& scaled, int page)
{
    if (!m_scroller) return;
    if (!scaled.isNull() && page!=m_currentPage) return;

    QPalette p = palette();
    p.setBrush(QPalette::Dark,QBrush(zF->global()->getBackgroundColor()));
    setPalette(p);
    Q_EMIT backgroundUpdated(zF->global()->getBackgroundColor());

    if (m_openedFile.isEmpty()) return;
    if (page<0 || page>=m_privPageCount) return;

    // Draw current page
    m_curPixmap = QImage();

    const bool isText = (!m_cacheLoaders.isEmpty()
                         && (!m_cacheLoaders.first().loader.isNull())
                         && (m_cacheLoaders.first().loader->isTextReader()));

    if (!m_curUnscaledPixmap.isNull()) {
        QSize scrollerSize = m_scroller->viewport()->size() - QSize(4,4);
        QSize targetSize = m_curUnscaledPixmap.size();

        m_curPixmap = m_curUnscaledPixmap;
        if (!m_curUnscaledPixmap.isNull() && m_curUnscaledPixmap.height()>0) {
            double pixAspect = static_cast<double>(m_curUnscaledPixmap.width()) /
                               static_cast<double>(m_curUnscaledPixmap.height());
            double myAspect = static_cast<double>(width()) /
                              static_cast<double>(height());

            if (!isText) {
                switch (m_zoomMode) {
                    case zmFit:
                        if ( pixAspect > myAspect ) { // the image is wider than widget -> resize by width
                            targetSize = QSize(scrollerSize.width(),
                                               qRound((static_cast<double>(scrollerSize.width()))/pixAspect));
                        } else {
                            targetSize = QSize(qRound((static_cast<double>(scrollerSize.height()))*pixAspect),
                                               scrollerSize.height());
                        }
                        break;
                    case zmWidth:
                        targetSize = QSize(scrollerSize.width(),
                                           qRound((static_cast<double>(scrollerSize.width()))/pixAspect));
                        break;
                    case zmHeight:
                        targetSize = QSize(qRound((static_cast<double>(scrollerSize.height()))*pixAspect),
                                           scrollerSize.height());
                        break;
                    case zmOriginal:
                        if (m_zoomAny>0)
                            targetSize = m_curUnscaledPixmap.size()*(static_cast<double>(m_zoomAny)/100.0);
                        break;
                }
            }

            if (targetSize!=m_curUnscaledPixmap.size()) {
                if (!scaled.isNull()) {
                    m_curPixmap = scaled;
                } else {
                    // fast inplace resampling
                    m_curPixmap = m_curUnscaledPixmap.scaled(targetSize,Qt::IgnoreAspectRatio,
                                                   Qt::FastTransformation);

                    if (zF->global()->getUseFineRendering()) {
                        Blitz::ScaleFilterType filter = Blitz::UndefinedFilter;
                        if (targetSize.width()>m_curUnscaledPixmap.width()) {
                            filter = zF->global()->getUpscaleFilter();
                        } else {
                            filter = zF->global()->getDownscaleFilter();
                        }

                        if (filter!=Blitz::UndefinedFilter) {
                            QImage image = m_curUnscaledPixmap;
                            m_resamplersPool.start([this,targetSize,filter,image,page](){
                                QElapsedTimer timer;
                                timer.start();

                                QImage res = ZGenericFuncs::resizeImage(image,targetSize,true,filter,
                                                                        page,&m_currentPage);

                                if (!res.isNull()) {
                                    qint64 elapsed = timer.elapsed();
                                    Q_EMIT requestRedrawPageEx(res, page);
                                    zF->global()->addFineRenderTime(elapsed);
                                }
                            });
                        }
                    }
                }
            }
        }
        setMinimumSize(m_curPixmap.width(),m_curPixmap.height());

        if (targetSize.height()<scrollerSize.height()) targetSize.setHeight(scrollerSize.height());
        if (targetSize.width()<scrollerSize.width()) targetSize.setWidth(scrollerSize.width());
        resize(targetSize);
    }
    update();
}

void ZMangaView::ownerResized(const QSize &size)
{
    Q_UNUSED(size)

    redrawPage();
}

void ZMangaView::changeMangaCoverCtx()
{
    if (m_openedFile.isEmpty()) return;
    if (m_currentPage<0 || m_currentPage>=m_privPageCount) return;

    Q_EMIT changeMangaCover(m_openedFile,m_currentPage);
}

void ZMangaView::exportPagesCtx()
{
    if (m_openedFile.isEmpty()) return;
    if (m_currentPage<0 || m_currentPage>=m_privPageCount) return;

    m_exportDialog.setPages(m_currentPage,m_privPageCount-m_currentPage);
    m_exportDialog.setParent(window(),Qt::Dialog);
    m_exportDialog.setExportDir(zF->global()->getSavedAuxSaveDir());
    if (m_exportDialog.exec()!=QDialog::Accepted) return;

    zF->global()->setSavedAuxSaveDir(m_exportDialog.getExportDir());

    int cnt = m_exportDialog.getPagesCount();
    if (cnt<1) return;

    auto *dlg = new QProgressDialog(this);
    dlg->setWindowModality(Qt::WindowModal);
    dlg->setAutoClose(false);
    dlg->setAutoReset(false);
    dlg->setWindowTitle(tr("Exporting pages..."));
    dlg->setRange(0,cnt);
    dlg->show();

    const int quality = m_exportDialog.getImageQuality();
    const int filenameLength = QString::number(cnt).length();
    const int currentPage = m_currentPage;
    const QString format = m_exportDialog.getImageFormat();
    const QString exportDir = m_exportDialog.getExportDir();
    const QString sourceFile = m_openedFile;

    QThread *th = QThread::create([cnt,quality,format,filenameLength,currentPage,exportDir,sourceFile,dlg]{
        QAtomicInteger<int> exportFileError(0);
        QAtomicInteger<int> progress(0);
        QVector<int> nums;
        nums.reserve(cnt);
        for (int i=0;i<cnt;i++)
            nums.append(i+currentPage);

        std::for_each(std::execution::par,nums.constBegin(),nums.constEnd(),
                      [&exportFileError,&progress,quality,format,filenameLength,exportDir,sourceFile,dlg]
                      (int pageNum){
            if (exportFileError.loadAcquire()>0 || dlg->wasCanceled()) return;

            QDir dir(exportDir);
            QString fname = dir.filePath(QSL("%1.%2")
                                         .arg(pageNum+1,filenameLength,ZDefaults::exportFilenameNumWidth,QChar(u'0'))
                                         .arg(format.toLower()));
            QScopedPointer<ZMangaLoader> zl(new ZMangaLoader());
            connect(zl.data(),&ZMangaLoader::gotError,[&exportFileError](const QString&msg){
                Q_UNUSED(msg)
                exportFileError.fetchAndAddRelease(1);
            });

            connect(zl.data(),&ZMangaLoader::gotPage,[&exportFileError,fname,quality,format]
                    (const QByteArray& page, const QImage& image, int num,
                    const QString& internalPath, const QUuid& threadID){
                Q_UNUSED(page)
                Q_UNUSED(num)
                Q_UNUSED(internalPath)
                Q_UNUSED(threadID)
                if (exportFileError.loadAcquire()>0) return;
                if (image.isNull()) return;
                if (!image.save(fname,format.toLatin1(),quality))
                    exportFileError.fetchAndAddRelease(1);
            });

            zl->openFile(sourceFile,0);
            if (exportFileError==0)
                zl->getPage(pageNum,true);
            zl->closeFile();
            progress.fetchAndAddRelease(1);
            const int progressValue = progress.loadAcquire();
            QMetaObject::invokeMethod(dlg,[progressValue,dlg]{
                if (!dlg->wasCanceled())
                    dlg->setValue(progressValue);
            },Qt::QueuedConnection);
        });

        const int exportFileErrorValue = exportFileError.loadAcquire();
        QMetaObject::invokeMethod(dlg,[dlg,exportFileErrorValue]{
            QString msg = tr("Pages export completed.");
            if (exportFileErrorValue>0)
                msg = tr("Error caught while saving image. Cannot create file. Check your permissions.");
            if (dlg->wasCanceled())
                msg = tr("Pages export aborted.");
            QMessageBox::information(dlg->parentWidget(),QGuiApplication::applicationDisplayName(),msg);
            dlg->close();
            dlg->deleteLater();
        });
    });
    connect(th,&QThread::finished,th,&QThread::deleteLater);
    th->setObjectName(QSL("Export_wrk"));
    th->start();
}

void ZMangaView::navFirst()
{
    setPage(0);
}

void ZMangaView::navPrev()
{
    if (m_currentPage>0)
        setPage(m_currentPage-1);
}

void ZMangaView::navNext()
{
    if (m_currentPage<m_privPageCount-1)
        setPage(m_currentPage+1);
}

void ZMangaView::navLast()
{
    setPage(m_privPageCount-1);
}

void ZMangaView::setZoomDynamic(bool state)
{
    m_zoomDynamic = state;
    redrawPage();
}

bool ZMangaView::getZoomDynamic() const
{
    return m_zoomDynamic;
}

void ZMangaView::setZoomAny(int comboIdx)
{
    auto *cb = qobject_cast<QComboBox *>(sender());
    if (cb==nullptr) return;
    QString s = cb->itemText(comboIdx);
    m_zoomAny = -1; // original zoom
    s.remove(QRegularExpression(QSL("[^0123456789]")));
    bool okconv = false;
    if (!s.isEmpty()) {
        m_zoomAny = s.toInt(&okconv);
        if (!okconv)
            m_zoomAny = -1;
    }
    redrawPage();
}

void ZMangaView::viewRotateCCW()
{
    m_rotation--;
    if (m_rotation<0) m_rotation = 3;
    displayCurrentPage();
    Q_EMIT rotationUpdated(m_rotation*M_PI_2);
}

void ZMangaView::viewRotateCW()
{
    m_rotation++;
    if (m_rotation>3) m_rotation = 0;
    displayCurrentPage();
    Q_EMIT rotationUpdated(m_rotation*M_PI_2);
}

void ZMangaView::loaderMsg(const QString &msg)
{
    Q_EMIT auxMessage(msg);
}

void ZMangaView::cacheGotPage(const QByteArray &page, const QImage &pageImage, int num,
                              const QString &internalPath, const QUuid &threadID)
{
    if (m_cacheLoaders.contains(ZLoaderHelper(threadID))) {
        m_cacheLoaders[m_cacheLoaders.indexOf(ZLoaderHelper(threadID))].delJob();
    }
    if (m_processingPages.contains(num))
        m_processingPages.removeOne(num);

    if ((!pageImage.isNull() && m_iCacheImages.contains(num)) ||
            (!page.isEmpty() && m_iCacheData.contains(num))) return;

    if (!page.isEmpty())
        m_iCacheData[num]=page;
    if (!pageImage.isNull())
        m_iCacheImages[num]=pageImage;

    m_pathCache[num]=internalPath;

    if (num==m_currentPage)
        displayCurrentPage();
}

void ZMangaView::cacheGotPageCount(int num, int preferred)
{
    if (m_privPageCount<=0) {
        m_privPageCount = num;
        setPage(preferred);
    }
}

void ZMangaView::cacheGotError(const QString &msg)
{
    QMessageBox::critical(this,QGuiApplication::applicationDisplayName(),msg);
}

void ZMangaView::cacheDropUnusable()
{
    ZIntVector toCache = cacheGetActivePages();
    for (auto it = m_iCacheImages.keyBegin(), end = m_iCacheImages.keyEnd(); it != end; ++it) {
        const int num = *it;
        if (!toCache.contains(num))
            m_iCacheImages.remove(num);
        if (m_pathCache.contains(num))
            m_pathCache.remove(num);
    }
    for (auto it = m_iCacheData.keyBegin(), end = m_iCacheData.keyEnd(); it != end; ++it) {
        const int num = *it;
        if (!toCache.contains(num))
            m_iCacheData.remove(num);
        if (m_pathCache.contains(num))
            m_pathCache.remove(num);
    }
}

void ZMangaView::cacheFillNearest()
{
    ZIntVector toCache = cacheGetActivePages();
    int idx = 0;
    while (idx<toCache.count()) {
        if (m_iCacheImages.contains(toCache.at(idx)) || m_iCacheData.contains(toCache.at(idx))) {
            toCache.removeAt(idx);
        } else {
            idx++;
        }
    }

    for (int i=0;i<toCache.count();i++) {
        if (!m_processingPages.contains(toCache.at(i))) {
            m_processingPages << toCache.at(i);
            cacheGetPage(toCache.at(i));
        }
    }
}

ZIntVector ZMangaView::cacheGetActivePages() const
{
    ZIntVector l;
    l.reserve(zF->global()->getCacheWidth()*2+1);

    if (m_currentPage==-1) return l;
    if (m_privPageCount<=0) {
        l.append(m_currentPage);
        return l;
    }

    int cacheRadius = 1;
    if (zF->global()->getCacheWidth()>=2) {
        if ((zF->global()->getCacheWidth() % 2)==0) {
            cacheRadius = zF->global()->getCacheWidth() / 2;
        } else {
            cacheRadius = (zF->global()->getCacheWidth()+1) / 2;
        }
    }

    l.append(m_currentPage); // load current page at first

    for (int i=0;i<cacheRadius;i++) {
        // first pages, then last pages
        const QList<int> pageNums ({ i, m_privPageCount-i-1 });
        for (const auto& num : pageNums) {
            if (!l.contains(num) && (num>=0) && (num<m_privPageCount))
                l.append(num);
        }
    }
    // pages around current page
    for (int i=(m_currentPage-cacheRadius);i<(m_currentPage+cacheRadius);i++) {
        if (i>=0 && i<m_privPageCount && !l.contains(i))
            l.append(i);
    }
    return l;
}
