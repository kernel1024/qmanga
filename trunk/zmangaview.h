#ifndef ZMANGAVIEW_H
#define ZMANGAVIEW_H

#include <QtCore>
#include <QtGui>
#include "global.h"
#include "zabstractreader.h"
#include "zmangaloader.h"

class ZMangaView : public QWidget
{
    Q_OBJECT
public:
    enum ZoomMode {
        Fit,
        Width,
        Height,
        Original
    };

protected:
    ZoomMode zoomMode;
    QPixmap curPixmap, curUmPixmap;
    QPoint zoomPos;

    QList<ZLoaderHelper> cacheLoaders;
    int privPageCount;

    QHash<int,QPixmap> iCachePixmaps;
    QHash<int,QByteArray> iCacheData;
    QHash<int,QString> pathCache;
    QIntList processingPages;

    QList<QSize> lastSizes;
    QList<int> lastFileSizes;

    void cacheDropUnusable();
    void cacheFillNearest();
    QIntList cacheGetActivePages();
    void displayCurrentPage();
    void cacheGetPage(int num);

public:
    int currentPage;
    QScrollArea* scroller;
    bool zoomDynamic;
    QString openedFile;
    int zoomAny;

    explicit ZMangaView(QWidget *parent = 0);
    ~ZMangaView();
    void setZoomMode(ZoomMode mode);
    ZoomMode getZoomMode();
    int getPageCount();
    void getPage(int num);
    
signals:
    void loadedPage(int num, QString msg);
    void doubleClicked();
    void keyPressed(int key);
    void averageSizes(QSize sz, qint64 fsz);

    // cache signals
    void cacheOpenFile(QString filename, int preferred);
    void cacheCloseFile();

public slots:
    void openFile(QString filename, int page = 0);
    void closeFile();
    void setPage(int page);
    void redrawPage();
    void ownerResized(const QSize& size);
    void minimizeWindowCtx();

    void navFirst();
    void navPrev();
    void navNext();
    void navLast();

    void setZoomFit();
    void setZoomWidth();
    void setZoomHeight();
    void setZoomOriginal();
    void setZoomDynamic(bool state);
    void setZoomAny(QString proc);

    // cache slots
    void cacheGotPage(const QByteArray& page, const int& num, const QString& internalPath,
                      const QUuid& threadID);
    void cacheGotPageCount(const int& num, const int& preferred);
    void cacheGotError(const QString& msg);

protected:
    void wheelEvent(QWheelEvent *event);
    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *);
    void keyPressEvent(QKeyEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
};

#endif // ZMANGAVIEW_H
