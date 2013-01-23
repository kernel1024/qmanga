#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QWidget>
#include <QMainWindow>
#include <QMenu>
#include <QLabel>

#include "zmangaview.h"
#include "zsearchtab.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    QMenu* bookmarksMenu;
    ZSearchTab* srcWidget;
    QLabel* lblSearchStatus;
    QLabel* lblAverageSizes;
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void centerWindow();
    
private:
    Ui::MainWindow *ui;
    bool fullScreen;

protected:
    void closeEvent(QCloseEvent * event);

public slots:
    void openAux();
    void openFromIndex(QString filename);
    void closeManga();
    void dispPage(int num, QString msg);
    void pageNumEdited();
    void switchFullscreen();
    void viewerKeyPressed(int key);
    void updateViewer();

    void updateBookmarks();
    void updateTitle();
    void createBookmark();
    void openBookmark();
    void openSearchTab();
    void helpAbout();
    void msgFromIndexer(QString msg);
    void msgFromMangaView(QSize sz, qint64 fsz);
};

#endif // MAINWINDOW_H
