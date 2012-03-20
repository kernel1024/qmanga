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
    ZMangaView* mangaView;
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private:
    Ui::MainWindow *ui;

protected:
    void closeEvent(QCloseEvent * event);

public slots:
    void openAux();
    void dispPage(int num);
};

#endif // MAINWINDOW_H
