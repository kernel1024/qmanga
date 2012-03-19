#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <quazip/quazip.h>
#include <quazip/quazipfile.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QFile af("test.zip");
    af.open(QIODevice::ReadOnly);
    QuaZip z(&af);
    z.open(QuaZip::mdUnzip);
    qDebug() << z.getEntriesCount();

    QuaZipFile zf(&z);
    for(bool more=z.goToFirstFile(); more; more=z.goToNextFile()) {
       QuaZipFileInfo zfi;
       z.getCurrentFileInfo(&zfi);
       zf.open(QIODevice::ReadOnly);

       manga.append(QImage::fromData(zf.readAll()));

       zf.close();


       // do something
    }


    z.close();
    af.close();

    ui->graphicsView->setImageList(&manga);

//    connect(ui->graphicsView)


}

MainWindow::~MainWindow()
{
    delete ui;
}
