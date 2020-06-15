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
        zmFit = 1,
        zmWidth = 2,
        zmHeight = 3,
        zmOriginal = 4
    };
    Q_ENUM(ZoomMode)

    enum MouseMode {
        mmOCR,
        mmPan,
        mmCrop
    };
    Q_ENUM(MouseMode)

private:
    Q_DISABLE_COPY(ZMangaView)

    ZoomMode m_zoomMode { zmFit };
    MouseMode m_mouseMode { mmOCR };
    bool m_zoomDynamic { false };
    int m_rotation { 0 };
    int m_currentPage { 0 };
    int m_scrollAccumulator { 0 };
    int m_privPageCount { 0 };
    int m_zoomAny { -1 };
    QImage m_curPixmap;
    QImage m_curUnscaledPixmap;
    QPoint m_drawPos;
    QPoint m_zoomPos;
    QPoint m_dragPos;
    QPoint m_copyPos;
    QPoint m_cropPos;
    QRubberBand* m_copySelection { nullptr };
    QRubberBand* m_cropSelection { nullptr };
    QRect m_cropRect;
    QPointer<QScrollArea> m_scroller;

    QString m_openedFile;

    QThreadPool m_resamplersPool;
    QList<ZLoaderHelper> m_cacheLoaders;

    QHash<int,QImage> m_iCacheImages;
    QHash<int,QByteArray> m_iCacheData;
    QHash<int,QString> m_pathCache;
    ZIntVector m_processingPages;

    QList<QSize> m_lastSizes;
    QList<int> m_lastFileSizes;

    ZExportDialog m_exportDialog;

    void cacheDropUnusable();
    void cacheFillNearest();
    ZIntVector cacheGetActivePages() const;
    void displayCurrentPage();
    void cacheGetPage(int num);   
    static ZExportWork exportMangaPage(const ZExportWork &item);

public:
    explicit ZMangaView(QWidget *parent = nullptr);
    ~ZMangaView() override;
    int getPageCount() const;
    void getPage(int num);
    void setMouseMode(ZMangaView::MouseMode mode);
    QString getOpenedFile() const;
    int getCurrentPage() const;
    void setScroller(QScrollArea* scroller);
    bool getZoomDynamic() const;

Q_SIGNALS:
    void loadedPage(int num, const QString &msg);
    void doubleClicked();
    void keyPressed(int key);
    void averageSizes(const QSize &sz, qint64 fsz);
    void minimizeRequested();
    void rotationUpdated(double angle);
    void auxMessage(const QString& msg);
    void cropUpdated(const QRect& crop);
    void backgroundUpdated(const QColor& color);
    void requestRedrawPageEx(const QImage &scaled, int page);
    void addOCRText(const QStringList& text);

    // cache signals
    void cacheOpenFile(const QString &filename, int preferred);
    void cacheCloseFile();

    // DB signals
    void changeMangaCover(const QString& fileName, int pageNum);
    void updateFileStats(const QString& fileName);

public Q_SLOTS:
    void setZoomMode(int mode);
    void openFile(const QString &filename);
    void openFileEx(const QString &filename, int page);
    void closeFile();
    void setPage(int page);
    void redrawPage();
    void redrawPageEx(const QImage &scaled, int page);
    void ownerResized(const QSize& size);
    void changeMangaCoverCtx();
    void exportPagesCtx();

    void navFirst();
    void navPrev();
    void navNext();
    void navLast();

    void setZoomDynamic(bool state);
    void setZoomAny(const QString &proc);

    void viewRotateCCW();
    void viewRotateCW();

    void loaderMsg(const QString& msg);

    // cache slots
    void cacheGotPage(const QByteArray& page, const QImage &pageImage, int num,
                      const QString& internalPath, const QUuid& threadID);
    void cacheGotPageCount(int num, int preferred);
    void cacheGotError(const QString& msg);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
};

#endif // ZMANGAVIEW_H
