#ifndef ZMANGAVIEW_H
#define ZMANGAVIEW_H

#include <QtCore>
#include <QtGui>
#include "global.h"
#include "zabstractreader.h"

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

private:
    ZAbstractReader* mReader;
    QHash<int,QImage> iCache;
    ZoomMode zoomMode;
    QPixmap curPixmap, curUmPixmap;
    QPoint zoomPos;

    void cacheDropUnusable();
    void cacheFillNearest();
    QIntList cacheGetActivePages();

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
    
signals:
    void loadPage(int num);
    void doubleClicked();
    void keyPressed(int key);

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

protected:
    void wheelEvent(QWheelEvent *event);
    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *);
    void keyPressEvent(QKeyEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
};

#endif // ZMANGAVIEW_H
