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

#include "zmangaview.h"
#include "zsearchtab.h"

class ZFSFile {
public:
    QString name;
    QString album;
    QString fileName;
    ZFSFile() = default;
    ~ZFSFile() = default;
    ZFSFile(const ZFSFile& other);
    ZFSFile(const QString &aName, const QString &aFileName, const QString &aAlbum);
    ZFSFile &operator=(const ZFSFile& other) = default;
};

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
    bool isFullScreenControlsVisible();
    void setZoomMode(int mode);
    void addContextMenuItems(QMenu *menu);
    
private:
    Q_DISABLE_COPY(ZMainWindow)

    Ui::ZMainWindow *ui;
    bool m_fullScreen { false };
    bool m_fullScreenControls { false };
    bool m_savedMaximized { false };
    QVector<ZFSFile> m_fsScannedFiles;
    QMutex m_fsAddFilesMutex;
    QRect m_savedGeometry;
    QMessageBox m_indexerMsgBox;

    QLabel* m_lblSearchStatus;
    QLabel* m_lblAverageSizes;
    QLabel* m_lblRotation;
    QLabel* m_lblCrop;
    QLabel* m_lblAverageFineRender;
    QButtonGroup* m_zoomGroup;

    void openAuxFile(const QString& filename);
    void updateControlsVisibility();

protected:
    void closeEvent(QCloseEvent * event) override;

Q_SIGNALS:
    void dbAddFiles(const QStringList& aFiles, const QString& album);
    void dbFindNewFiles();
    void dbAddIgnoredFiles(const QStringList& files);

public Q_SLOTS:
    void openAux();
    void openClipboard();
    void openFromIndex(const QString &filename);
    void closeManga();
    void dispPage(int num, const QString &msg);
    void pageNumEdited();
    void switchFullscreen();
    void switchFullscreenControls();
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
    void fsUpdateFileList();
    void fsResultsMenuCtx(const QPoint& pos);
    void fsResultsCtxApplyAlbum();
    void fsFindNewFiles();
    void fsFoundNewFiles(const QStringList& files);
    void fsAddIgnoredFiles();
};

#endif // MAINWINDOW_H
