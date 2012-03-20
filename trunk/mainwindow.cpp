#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "zglobal.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    if (zGlobal==NULL)
        zGlobal = new ZGlobal(this);

    ui->btnOpen->setIcon(QIcon(":/img/document-open.png"));
    ui->btnNavFirst->setIcon(QIcon(":/img/go-first.png"));
    ui->btnNavPrev->setIcon(QIcon(":/img/go-previous.png"));
    ui->btnNavNext->setIcon(QIcon(":/img/go-next.png"));
    ui->btnNavLast->setIcon(QIcon(":/img/go-last.png"));
    ui->btnZoomFit->setIcon(QIcon(":/img/zoom-fit-best.png"));
    ui->btnZoomWidth->setIcon(QIcon(":/img/zoom-fit-width.png"));
    ui->btnZoomHeight->setIcon(QIcon(":/img/zoom-fit-height.png"));
    ui->btnZoomOriginal->setIcon(QIcon(":/img/zoom-original.png"));
    ui->btnZoomDynamic->setIcon(QIcon(":/img/zoom-draw.png"));

    connect(ui->actionExit,SIGNAL(triggered()),this,SLOT(close()));
    connect(ui->actionOpen,SIGNAL(triggered()),this,SLOT(openAux()));
    connect(ui->btnOpen,SIGNAL(clicked()),this,SLOT(openAux()));
    connect(ui->mangaView,SIGNAL(loadPage(int)),this,SLOT(dispPage(int)));

    connect(ui->btnNavFirst,SIGNAL(clicked()),ui->mangaView,SLOT(navFirst()));
    connect(ui->btnNavPrev,SIGNAL(clicked()),ui->mangaView,SLOT(navPrev()));
    connect(ui->btnNavNext,SIGNAL(clicked()),ui->mangaView,SLOT(navNext()));
    connect(ui->btnNavLast,SIGNAL(clicked()),ui->mangaView,SLOT(navLast()));

    connect(ui->btnZoomFit,SIGNAL(clicked()),ui->mangaView,SLOT(setZoomFit()));
    connect(ui->btnZoomHeight,SIGNAL(clicked()),ui->mangaView,SLOT(setZoomHeight()));
    connect(ui->btnZoomWidth,SIGNAL(clicked()),ui->mangaView,SLOT(setZoomWidth()));
    connect(ui->btnZoomOriginal,SIGNAL(clicked()),ui->mangaView,SLOT(setZoomOriginal()));
    connect(ui->btnZoomDynamic,SIGNAL(toggled(bool)),ui->mangaView,SLOT(setZoomDynamic(bool)));

    connect(ui->editMySqlLogin,SIGNAL(textChanged(QString)),this,SLOT(stLoginChanged(QString)));
    connect(ui->editMySqlPassword,SIGNAL(textChanged(QString)),this,SLOT(stPassChanged(QString)));
    connect(ui->spinCacheWidth,SIGNAL(valueChanged(int)),this,SLOT(stCacheWidthChanged(int)));
    connect(ui->comboFilter,SIGNAL(currentIndexChanged(int)),this,SLOT(stFilterChanged(int)));

    ui->mangaView->scroller = ui->scrollArea;
    zGlobal->loadSettings();
    updateSettingsPage();
    dispPage(-1);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (zGlobal!=NULL)
        zGlobal->saveSettings();
    event->accept();
}

void MainWindow::openAux()
{
    QString filename = zGlobal->getOpenFileNameD(this,tr("Open manga archive"),zGlobal->savedAuxOpenDir);
    if (!filename.isEmpty()) {
        QFileInfo fi(filename);
        zGlobal->savedAuxOpenDir = fi.path();

        ui->mangaView->openFile(filename);
    }
}

void MainWindow::dispPage(int num)
{
    if (num<0 || ui->mangaView->getPageCount()<=0)
        ui->lblPosition->clear();
    else
        ui->lblPosition->setText(tr("%1 / %2").arg(num+1).arg(ui->mangaView->getPageCount()));
}

void MainWindow::stLoginChanged(QString text)
{
    zGlobal->mysqlUser=text;
}

void MainWindow::stPassChanged(QString text)
{
    zGlobal->mysqlPassword=text;
}

void MainWindow::stCacheWidthChanged(int num)
{
    zGlobal->cacheWidth=num;
}

void MainWindow::stFilterChanged(int num)
{
    switch (num) {
        case 0: zGlobal->resizeFilter = ZGlobal::Nearest; break;
        case 1: zGlobal->resizeFilter = ZGlobal::Bilinear; break;
        case 2: zGlobal->resizeFilter = ZGlobal::Lanczos; break;
    }
}

void MainWindow::updateSettingsPage()
{
    ui->editMySqlLogin->setText(zGlobal->mysqlUser);
    ui->editMySqlPassword->setText(zGlobal->mysqlPassword);
    ui->spinCacheWidth->setValue(zGlobal->cacheWidth);
    switch (zGlobal->resizeFilter) {
        case ZGlobal::Nearest: ui->comboFilter->setCurrentIndex(0); break;
        case ZGlobal::Bilinear: ui->comboFilter->setCurrentIndex(1); break;
        case ZGlobal::Lanczos: ui->comboFilter->setCurrentIndex(2); break;
    }
}
