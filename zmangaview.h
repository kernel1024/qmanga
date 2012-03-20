#ifndef ZMANGAVIEW_H
#define ZMANGAVIEW_H

#include <QtCore>
#include <QtGui>
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
    QPixmap currentPixmap;

    ZAbstractReader* readerFactory(QString filename);

    void cacheDropUnusable();
    void cacheFillNearest();
    QIntList cacheGetActivePages();

public:
    QScrollArea* scroller;
    int currentPage;
    explicit ZMangaView(QWidget *parent = 0);
    ~ZMangaView();
    void setZoomMode(ZoomMode mode);
    ZoomMode getZoomMode();
    int getPageCount();
    
signals:
    void loadPage(int num);

public slots:
    void openFile(QString filename);
    void closeFile();
    void setPage(int page);
    void redrawPage();

    void navFirst();
    void navPrev();
    void navNext();
    void navLast();

    void setZoomFit();
    void setZoomWidth();
    void setZoomHeight();
    void setZoomOriginal();
    void setZoomDynamic(bool state);

protected:
    void wheelEvent(QWheelEvent *event);
    void paintEvent(QPaintEvent *);
};

#endif // ZMANGAVIEW_H
