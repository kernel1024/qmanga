#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QMainWindow>
#include <QMenu>
#include <QLabel>
#include <QList>
#include <QMutex>
#include <QMessageBox>
#include <QButtonGroup>
#include <QPointer>

#include "zmangaview.h"
#include "zsearchtab.h"
#include "zfilesystemscannermodel.h"

namespace Ui {
class ZMainWindow;
}

class ZMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit ZMainWindow(QWidget *parent = nullptr);
    ~ZMainWindow() override;
    void centerWindow(bool moveWindow);
    bool isMangaOpened();
    bool isFullScreenControlsVisible() const;
    void setZoomMode(int mode);
    void addContextMenuItems(QMenu *menu);
    
private:
    Q_DISABLE_COPY(ZMainWindow)

    Ui::ZMainWindow *ui;
    bool m_fullScreen { false };
    bool m_fullScreenControls { false };
    bool m_savedMaximized { false };
    QRect m_savedGeometry;
    QMessageBox m_indexerMsgBox;

    QPointer<QLabel> m_lblSearchStatus;
    QPointer<QLabel> m_lblAverageSizes;
    QPointer<QLabel> m_lblRotation;
    QPointer<QLabel> m_lblCrop;
    QPointer<QLabel> m_lblAverageFineRender;
    QPointer<QButtonGroup> m_zoomGroup;
    QPointer<ZFilesystemScannerModel> m_fsModel;

    void openAuxFile(const QString& filename);
    void fsAddFilesWorker(const QStringList& auxList);

protected:
    void closeEvent(QCloseEvent * event) override;

Q_SIGNALS:
    void dbAddFiles(const QStringList& aFiles, const QString& album);
    void dbFindNewFiles();
    void dbAddIgnoredFiles(const QStringList& files);

public Q_SLOTS:
    void openAux();
    void openAuxImagesDir();
    void openClipboard();
    void openFromIndex(const QString &filename);
    void closeManga();
    void newWindow();
    void dispPage(int num, const QString &msg);
    void pageNumEdited();
    void switchFullscreen();
    void switchFullscreenControls();
    void updateControlsVisibility();
    void viewerKeyPressed(int key);
    void updateViewer();
    void rotationUpdated(double angle);
    void cropUpdated(const QRect& crop);
    void fastScroll(int page);
    void updateFastScrollPosition();
    void tabChanged(int idx);
    void changeMouseMode(int mode);
    void viewerBackgroundUpdated(const QColor& color);

    void updateBookmarks();
    void updateTitle();
    void createBookmark();
    void openBookmark();
    void openSearchTab();
    void helpAbout();
    void auxMessage(const QString &msg);
    void msgFromMangaView(const QSize &sz, qint64 fsz);

    void fsAddFiles();
    void fsCheckAvailability();
    void fsDelFiles();
    void fsNewFilesAdded();
    void fsFindNewFiles();
    void fsFoundNewFiles(const QStringList& files);
    void fsAddIgnoredFiles();
};

#endif // MAINWINDOW_H
