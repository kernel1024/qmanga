#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QtGui>
#include "zmangaview.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    QMenu* bookmarksMenu;
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
    void closeManga();
    void dispPage(int num);
    void goFullscreen();

    void updateBookmarks();
    void updateTitle();
    void createBookmark();
    void openBookmark();
};

#endif // MAINWINDOW_H
