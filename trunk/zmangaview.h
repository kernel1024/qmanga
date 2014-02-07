#ifndef ZMANGAVIEW_H
#define ZMANGAVIEW_H

#include <QWidget>
#include <QRubberBand>
#include <QPixmap>
#include <QScrollArea>

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
    int rotation;
    QPixmap curPixmap, curUmPixmap;
    QPoint drawPos;
    QPoint zoomPos;
    QPoint dragPos;
    QPoint copyPos;
    QRubberBand* copySelection;

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
    void minimizeRequested();
    void closeFileRequested();
    void rotationUpdated(int degree);

    // cache signals
    void cacheOpenFile(QString filename, int preferred);
    void cacheCloseFile();

    // DB signals
    void changeMangaCover(const QString& fileName, const int pageNum);
    void updateFileStats(const QString& fileName);

public slots:
    void openFile(QString filename, int page = 0);
    void closeFile();
    void setPage(int page);
    void redrawPage();
    void ownerResized(const QSize& size);
    void minimizeWindowCtx();
    void closeFileCtx();
    void changeMangaCoverCtx();

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

    void viewRotateCCW();
    void viewRotateCW();

    void asyncMsg(const QString& msg);

    // cache slots
    void cacheGotPage(const QByteArray& page, const int& num, const QString& internalPath,
                      const QUuid& threadID);
    void cacheGotPageCount(const int& num, const int& preferred);
    void cacheGotError(const QString& msg);

protected:
    void wheelEvent(QWheelEvent *event);
    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void keyPressEvent(QKeyEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
};

#endif // ZMANGAVIEW_H
