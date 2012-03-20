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

    mangaView = new ZMangaView(this);
    mangaView->scroller = ui->scrollArea;
    QGridLayout* grid = new QGridLayout();
    grid->addWidget(mangaView,1,1,1,1);
    grid->addItem(new QSpacerItem(20,40,QSizePolicy::Minimum,QSizePolicy::Expanding),0,1,1,1);
    grid->addItem(new QSpacerItem(20,40,QSizePolicy::Minimum,QSizePolicy::Expanding),2,1,1,1);
    grid->addItem(new QSpacerItem(40,20,QSizePolicy::Expanding,QSizePolicy::Minimum),1,0,1,1);
    grid->addItem(new QSpacerItem(40,20,QSizePolicy::Expanding,QSizePolicy::Minimum),1,2,1,1);
    ui->scrollArea->setLayout(grid);

    connect(ui->actionExit,SIGNAL(triggered()),this,SLOT(close()));
    connect(ui->actionOpen,SIGNAL(triggered()),this,SLOT(openAux()));
    connect(ui->btnOpen,SIGNAL(clicked()),this,SLOT(openAux()));
    connect(mangaView,SIGNAL(loadPage(int)),this,SLOT(dispPage(int)));

    connect(ui->btnNavFirst,SIGNAL(clicked()),mangaView,SLOT(navFirst()));
    connect(ui->btnNavPrev,SIGNAL(clicked()),mangaView,SLOT(navPrev()));
    connect(ui->btnNavNext,SIGNAL(clicked()),mangaView,SLOT(navNext()));
    connect(ui->btnNavLast,SIGNAL(clicked()),mangaView,SLOT(navLast()));

    connect(ui->btnZoomFit,SIGNAL(clicked()),mangaView,SLOT(setZoomFit()));
    connect(ui->btnZoomHeight,SIGNAL(clicked()),mangaView,SLOT(setZoomHeight()));
    connect(ui->btnZoomWidth,SIGNAL(clicked()),mangaView,SLOT(setZoomWidth()));
    connect(ui->btnZoomOriginal,SIGNAL(clicked()),mangaView,SLOT(setZoomOriginal()));
    connect(ui->btnZoomDynamic,SIGNAL(toggled(bool)),mangaView,SLOT(setZoomDynamic(bool)));

    zGlobal->loadSettings();
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
    QString filename = zGlobal->getOpenFileNameD(this,tr("Open manga archive"));
    if (!filename.isEmpty())
        mangaView->openFile(filename);
}

void MainWindow::dispPage(int num)
{
    if (num<0 || mangaView->getPageCount()<=0)
        ui->lblPosition->clear();
    else
        ui->lblPosition->setText(tr("%1 / %2").arg(num+1).arg(mangaView->getPageCount()));
}
