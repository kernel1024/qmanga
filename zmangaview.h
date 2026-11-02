#ifndef ZMANGAVIEW_H
#define ZMANGAVIEW_H

#include <QWidget>
#include <QRubberBand>
#include <QPixmap>
#include <QScrollArea>
#include <QThreadPool>

#include "global.h"
#include "readers/zabstractreader.h"
#include "ocr/abstractocr.h"
#include "ocr/ocrresult.h"
#include "zmangaloader.h"
#include "zexportdialog.h"

#ifdef QT_DBUS_LIB
#include "auxtranslator_interface.h"
#endif

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
    ZAbstractOCR *m_previousOCR { nullptr };
    QRect m_cropRect;
    QPointer<QScrollArea> m_scroller;

    QString m_openedFile;
    QString m_filePassword;

    QThreadPool m_resamplersPool;
    QList<ZLoaderHelper> m_cacheLoaders;

    QHash<int,QImage> m_iCacheImages;
    QHash<int,QByteArray> m_iCacheData;
    QHash<int,QString> m_pathCache;
    ZIntVector m_processingPages;

    QList<QSize> m_lastSizes;
    QList<int> m_lastFileSizes;

    ZExportDialog m_exportDialog;

    // OCR overlay
    Z::ZOCRPageResult m_ocrPageResult;

    int m_ocrPageNumber { -1 };
    int m_ocrHoveredRegion { -1 };

    bool m_showOCROverlay { false };

    QHash<quint64, int> m_pendingTranslations;
    quint64 m_nextTranslationId = 0;
    quint64 m_pageGeneration = 0;

    void updateOCRTooltip();

    // Convert an image-space polygon into widget coordinates.
    QPolygonF mapOCRPolygon(const QPolygonF &polygon) const;

    // Find a region under a widget-space point.
    int ocrRegionAt(const QPoint &pos) const;

    // Invalidate the current OCR overlay.
    void clearOCROverlay();

#ifdef QT_DBUS_LIB
    OrgKernel1024JpreaderAuxtranslatorInterface
        *m_pageTranslator { nullptr };
    void startPageTranslation();
#endif

    void cacheDropUnusable();
    void cacheFillNearest();
    ZIntVector cacheGetActivePages() const;
    void displayCurrentPage();
    void cacheGetPage(int num);
    void handleOCRRequest(const QImage &image);

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
    void cacheOpenFile(const QString &filename, const QString &password, int preferred);
    void cacheCloseFile();

    // DB signals
    void changeMangaCover(const QString& fileName, const QString& password, int pageNum);
    void updateFileStats(const QString& fileName, const QString& password);

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
    void setZoomAny(int comboIdx);

    void viewRotateCCW();
    void viewRotateCW();

    void fullPageOCR();
    void ocrFinished(const QString &text);
    void ocrPageFinished(const Z::ZOCRPageResult &result, int pageNumber);

    void loaderMsg(const QString& msg);

#ifdef QT_DBUS_LIB
    void pageTranslationReceived(qulonglong requestId, const QString &text);
#endif

    // cache slots
    void cacheGotPage(const QByteArray& page, const QImage &pageImage, int num,
                      const QString& internalPath, const QUuid& threadID);
    void cacheGotPageCount(int num, int preferred);

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
