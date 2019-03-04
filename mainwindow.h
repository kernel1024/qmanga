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
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void centerWindow(bool moveWindow);
    bool isMangaOpened();
    
private:
    Ui::MainWindow *ui;
    bool fullScreen;
    QVector<ZFSFile> fsScannedFiles;
    QMutex fsAddFilesMutex;
    bool savedMaximized;
    QPoint savedPos;
    QSize savedSize;
    QMessageBox indexerMsgBox;
    void openAuxFile(const QString& filename);

protected:
    void closeEvent(QCloseEvent * event);
    bool eventFilter(QObject * obj, QEvent * event);

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
    void viewerKeyPressed(int key);
    void updateViewer();
    void rotationUpdated(int degree);
    void cropUpdated(const QRect& crop);
    void fastScroll(int page);
    void updateFastScrollPosition();
    void tabChanged(int idx);
    void changeMouseMode(bool state);
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

class ZPopupFrame : public QFrame {
    Q_OBJECT
private:
    MainWindow* mwnd;

public:
    explicit ZPopupFrame(QWidget* parent = nullptr);
    void setMainWindow(MainWindow* wnd);

protected:
    void enterEvent(QEvent* event);
    void leaveEvent(QEvent* event);
    void hideChildren();
    void showChildren();

signals:
    void showWidget();
    void hideWidget();

};

#endif // MAINWINDOW_H
