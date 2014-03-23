#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QMainWindow>
#include <QMenu>
#include <QLabel>
#include <QList>
#include <QMutex>

#include "zmangaview.h"
#include "zsearchtab.h"

class ZFSFile {
public:
    QString name;
    QString album;
    QString fileName;
    ZFSFile();
    ZFSFile(QString aName, QString aFileName, QString aAlbum);
    ZFSFile &operator=(const ZFSFile& other);
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
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void centerWindow();
    bool isMangaOpened();
    
private:
    Ui::MainWindow *ui;
    bool fullScreen;
    QList<ZFSFile> fsScannedFiles;
    QMutex fsAddFilesMutex;

protected:
    void closeEvent(QCloseEvent * event);
    void resizeEvent(QResizeEvent * event);
    bool eventFilter(QObject * obj, QEvent * event);

signals:
    void dbAddFiles(const QStringList& aFiles, const QString& album);

public slots:
    void openAux();
    void openClipboard();
    void openFromIndex(QString filename);
    void closeManga();
    void dispPage(int num, QString msg);
    void pageNumEdited();
    void switchFullscreen();
    void viewerKeyPressed(int key);
    void updateViewer();
    void rotationUpdated(int degree);
    void fastScroll(int page);
    void updateFastScrollPosition();

    void updateBookmarks();
    void updateTitle();
    void createBookmark();
    void openBookmark();
    void openSearchTab();
    void helpAbout();
    void msgFromIndexer(QString msg);
    void msgFromMangaView(QSize sz, qint64 fsz);

    void fsAddFiles();
    void fsCheckAvailability();
    void fsDelFiles();
    void fsNewFilesAdded();
    void fsUpdateFileList();
    void fsResultsMenuCtx(const QPoint& pos);
    void fsResultsCtxApplyAlbum();
};

class ZPopupFrame : public QFrame {
    Q_OBJECT
public:
    explicit ZPopupFrame(QWidget* parent = 0);
    void setMainWindow(MainWindow* wnd);
protected:
    MainWindow* mwnd;
    void enterEvent(QEvent* event);
    void leaveEvent(QEvent* event);
    void hideChildren();
    void showChildren();
signals:
    void showWidget();
    void hideWidget();
};

#endif // MAINWINDOW_H
