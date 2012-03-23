#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QtGui>
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
    void dispPage(int num);
    void switchFullscreen();
    void viewerKeyPressed(int key);
    void updateViewer();

    void updateBookmarks();
    void updateTitle();
    void createBookmark();
    void openBookmark();
    void helpAbout();
    void msgFromIndexer(QString msg);
};

#endif // MAINWINDOW_H
