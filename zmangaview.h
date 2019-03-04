#ifndef ZMANGAVIEW_H
#define ZMANGAVIEW_H

#include <QWidget>
#include <QRubberBand>
#include <QPixmap>
#include <QScrollArea>
#include <QThreadPool>

#include "global.h"
#include "zabstractreader.h"
#include "zmangaloader.h"
#include "zexportdialog.h"

class ZMangaView : public QWidget
{
    Q_OBJECT
public:
    enum ZoomMode {
        zmFit,
        zmWidth,
        zmHeight,
        zmOriginal
    };

    enum MouseMode {
        mmOCR,
        mmPan,
        mmCrop
    };

private:
    ZoomMode zoomMode;
    MouseMode mouseMode;
    int rotation;
    QImage curPixmap, curUmPixmap;
    QPoint drawPos;
    QPoint zoomPos;
    QPoint dragPos;
    QPoint copyPos;
    QPoint cropPos;
    QRubberBand* copySelection;
    QRubberBand* cropSelection;
    QRect cropRect;

    QList<ZLoaderHelper> cacheLoaders;
    int privPageCount;

    QHash<int,QImage> iCacheImages;
    QHash<int,QByteArray> iCacheData;
    QHash<int,QString> pathCache;
    QIntVector processingPages;

    QList<QSize> lastSizes;
    QList<int> lastFileSizes;

    ZExportDialog exportDialog;

    QThreadPool resamplersPool;

    int scrollAccumulator;

    void cacheDropUnusable();
    void cacheFillNearest();
    QIntVector cacheGetActivePages();
    void displayCurrentPage();
    void cacheGetPage(int num);   
    static ZExportWork exportMangaPage(const ZExportWork &item);

public:
    int currentPage;
    QScrollArea* scroller;
    bool zoomDynamic;
    QString openedFile;
    int zoomAny;

    explicit ZMangaView(QWidget *parent = nullptr);
    ~ZMangaView();
    void setZoomMode(ZoomMode mode);
    int getPageCount();
    void getPage(int num);
    void setMouseMode(MouseMode mode);
    
signals:
    void loadedPage(int num, const QString &msg);
    void doubleClicked();
    void keyPressed(int key);
    void averageSizes(const QSize &sz, qint64 fsz);
    void minimizeRequested();
    void closeFileRequested();
    void rotationUpdated(int degree);
    void auxMessage(const QString& msg);
    void cropUpdated(const QRect& crop);
    void backgroundUpdated(const QColor& color);
    void requestRedrawPageEx(const QImage &scaled, int page);

    // cache signals
    void cacheOpenFile(const QString &filename, int preferred);
    void cacheCloseFile();

    // DB signals
    void changeMangaCover(const QString& fileName, const int pageNum);
    void updateFileStats(const QString& fileName);

public slots:
    void openFile(const QString &filename);
    void openFileEx(const QString &filename, int page);
    void closeFile();
    void setPage(int page);
    void redrawPage();
    void redrawPageEx(const QImage &scaled, int page);
    void ownerResized(const QSize& size);
    void minimizeWindowCtx();
    void closeFileCtx();
    void changeMangaCoverCtx();
    void exportPagesCtx();

    void navFirst();
    void navPrev();
    void navNext();
    void navLast();

    void setZoomFit();
    void setZoomWidth();
    void setZoomHeight();
    void setZoomOriginal();
    void setZoomDynamic(bool state);
    void setZoomAny(const QString &proc);

    void viewRotateCCW();
    void viewRotateCW();

    void loaderMsg(const QString& msg);

    // cache slots
    void cacheGotPage(const QByteArray& page, const QImage &pageImage, const int& num,
                      const QString& internalPath, const QUuid& threadID);
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
