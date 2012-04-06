#ifndef ZMANGAVIEW_H
#define ZMANGAVIEW_H

#include <QtCore>
#include <QtGui>
#include "global.h"
#include "zabstractreader.h"
#include "zmangacache.h"

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
    ZoomMode zoomMode;
    QPixmap curPixmap, curUmPixmap;
    QPoint zoomPos;

    QThread* threadCache;
    int privPageCount;

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
    void loadedPage(int num, QString msg);
    void doubleClicked();
    void keyPressed(int key);

    // cache signals
    void cacheOpenFile(QString filename, int preferred);
    void cacheGetPage(int num);
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
    void cacheGotPage(const QImage& page, const int& num, const QString& internalPath);
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
