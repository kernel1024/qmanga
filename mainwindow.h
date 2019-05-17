#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QMainWindow>
#include <QMenu>
#include <QLabel>
#include <QList>
#include <QMutex>
#include <QMessageBox>

#include "zmangaview.h"
#include "zsearchtab.h"

class ZFSFile {
public:
    QString name;
    QString album;
    QString fileName;
    ZFSFile();
    ZFSFile(const ZFSFile& other);
    ZFSFile(const QString &aName, const QString &aFileName, const QString &aAlbum);
    ZFSFile &operator=(const ZFSFile& other) = default;
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    QMenu* bookmarksMenu;
    ZSearchTab* searchTab;
    QLabel* lblSearchStatus;
    QLabel* lblAverageSizes;
    QLabel* lblRotation;
    QLabel* lblCrop;
    QLabel* lblAverageFineRender;
    QFrame* fastScrollPanel;
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void centerWindow(bool moveWindow);
    bool isMangaOpened();
    bool isFullScreenControlsVisible();
    
private:
    Ui::MainWindow *ui;
    bool fullScreen;
    bool fullScreenControls;
    QVector<ZFSFile> fsScannedFiles;
    QMutex fsAddFilesMutex;
    bool savedMaximized;
    QRect savedGeometry;
    QMessageBox indexerMsgBox;
    void openAuxFile(const QString& filename);
    void updateControlsVisibility();

protected:
    void closeEvent(QCloseEvent * event);

signals:
    void dbAddFiles(const QStringList& aFiles, const QString& album);
    void dbFindNewFiles();
    void dbAddIgnoredFiles(const QStringList& files);

public slots:
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
    void rotationUpdated(int degree);
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
