#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "zglobal.h"
#include "bookmarkdlg.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowIcon(QIcon(":/img/Alien9.png"));

    bookmarksMenu = ui->menuBookmarks;
    srcWidget = ui->srcWidget;
    fullScreen = false;

    if (zGlobal==NULL)
        zGlobal = new ZGlobal(this);

    QApplication::setWheelScrollLines(1);

    lblSearchStatus = new QLabel(tr("Ready"));
    statusBar()->addPermanentWidget(lblSearchStatus);

    int btnSize = 20;
    ui->btnOpen->setIcon(QIcon(":/img/document-open.png"));
    ui->btnOpen->setIconSize(QSize(btnSize,btnSize));
    ui->btnNavFirst->setIcon(QIcon(":/img/go-first.png"));
    ui->btnNavFirst->setIconSize(QSize(btnSize,btnSize));
    ui->btnNavPrev->setIcon(QIcon(":/img/go-previous.png"));
    ui->btnNavPrev->setIconSize(QSize(btnSize,btnSize));
    ui->btnNavNext->setIcon(QIcon(":/img/go-next.png"));
    ui->btnNavNext->setIconSize(QSize(btnSize,btnSize));
    ui->btnNavLast->setIcon(QIcon(":/img/go-last.png"));
    ui->btnNavLast->setIconSize(QSize(btnSize,btnSize));
    ui->btnZoomFit->setIcon(QIcon(":/img/zoom-fit-best.png"));
    ui->btnZoomFit->setIconSize(QSize(btnSize,btnSize));
    ui->btnZoomWidth->setIcon(QIcon(":/img/zoom-fit-width.png"));
    ui->btnZoomWidth->setIconSize(QSize(btnSize,btnSize));
    ui->btnZoomHeight->setIcon(QIcon(":/img/zoom-fit-height.png"));
    ui->btnZoomHeight->setIconSize(QSize(btnSize,btnSize));
    ui->btnZoomOriginal->setIcon(QIcon(":/img/zoom-original.png"));
    ui->btnZoomOriginal->setIconSize(QSize(btnSize,btnSize));
    ui->btnZoomDynamic->setIcon(QIcon(":/img/zoom-draw.png"));
    ui->btnZoomDynamic->setIconSize(QSize(btnSize,btnSize));

    connect(ui->actionExit,SIGNAL(triggered()),this,SLOT(close()));
    connect(ui->actionOpen,SIGNAL(triggered()),this,SLOT(openAux()));
    connect(ui->actionClose,SIGNAL(triggered()),this,SLOT(closeManga()));
    connect(ui->actionSettings,SIGNAL(triggered()),zGlobal,SLOT(settingsDlg()));
    connect(ui->actionAddBookmark,SIGNAL(triggered()),this,SLOT(createBookmark()));
    connect(ui->actionAbout,SIGNAL(triggered()),this,SLOT(helpAbout()));
    connect(ui->actionFullscreen,SIGNAL(triggered()),this,SLOT(switchFullscreen()));
    connect(ui->actionMinimize,SIGNAL(triggered()),this,SLOT(showMinimized()));
    connect(ui->actionSaveSettings,SIGNAL(triggered()),zGlobal,SLOT(saveSettings()));

    connect(ui->btnOpen,SIGNAL(clicked()),this,SLOT(openAux()));
    connect(ui->mangaView,SIGNAL(loadedPage(int,QString)),this,SLOT(dispPage(int,QString)));
    connect(ui->scrollArea,SIGNAL(sizeChanged(QSize)),ui->mangaView,SLOT(ownerResized(QSize)));
    connect(ui->comboZoom,SIGNAL(currentIndexChanged(QString)),ui->mangaView,SLOT(setZoomAny(QString)));
    connect(ui->mangaView,SIGNAL(doubleClicked()),this,SLOT(switchFullscreen()));
    connect(ui->mangaView,SIGNAL(keyPressed(int)),this,SLOT(viewerKeyPressed(int)));
    connect(ui->srcWidget,SIGNAL(mangaDblClick(QString)),this,SLOT(openFromIndex(QString)));
    connect(ui->srcWidget,SIGNAL(statusBarMsg(QString)),this,SLOT(msgFromIndexer(QString)));

    connect(ui->btnNavFirst,SIGNAL(clicked()),ui->mangaView,SLOT(navFirst()));
    connect(ui->btnNavPrev,SIGNAL(clicked()),ui->mangaView,SLOT(navPrev()));
    connect(ui->btnNavNext,SIGNAL(clicked()),ui->mangaView,SLOT(navNext()));
    connect(ui->btnNavLast,SIGNAL(clicked()),ui->mangaView,SLOT(navLast()));

    connect(ui->btnZoomFit,SIGNAL(clicked()),ui->mangaView,SLOT(setZoomFit()));
    connect(ui->btnZoomHeight,SIGNAL(clicked()),ui->mangaView,SLOT(setZoomHeight()));
    connect(ui->btnZoomWidth,SIGNAL(clicked()),ui->mangaView,SLOT(setZoomWidth()));
    connect(ui->btnZoomOriginal,SIGNAL(clicked()),ui->mangaView,SLOT(setZoomOriginal()));
    connect(ui->btnZoomDynamic,SIGNAL(toggled(bool)),ui->mangaView,SLOT(setZoomDynamic(bool)));

    ui->mangaView->scroller = ui->scrollArea;
    zGlobal->loadSettings();
    if (!isMaximized())
        centerWindow();
    dispPage(-1,QString());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::centerWindow()
{
    int screen = 0;
    QWidget *w = window();
    QDesktopWidget *desktop = QApplication::desktop();
    if (w) {
        screen = desktop->screenNumber(w);
    } else if (desktop->isVirtualDesktop()) {
        screen = desktop->screenNumber(QCursor::pos());
    } else {
        screen = desktop->screenNumber(this);
    }

    QRect rect(desktop->availableGeometry(screen));
    int h = 80*rect.height()/100;
    QSize nw(135*h/100,h);
    if (nw.width()<1000) nw.setWidth(80*rect.width()/100);
    resize(nw);
    move(rect.width()/2 - frameGeometry().width()/2,
         rect.height()/2 - frameGeometry().height()/2);
    srcWidget->updateSplitters();
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

        ui->tabWidget->setCurrentIndex(0);
        ui->mangaView->openFile(filename);
    }
}

void MainWindow::openFromIndex(QString filename)
{
    ui->tabWidget->setCurrentIndex(0);
    ui->mangaView->openFile(filename);
}

void MainWindow::closeManga()
{
    ui->mangaView->closeFile();
}

void MainWindow::dispPage(int num, QString msg)
{
    updateTitle();
    if (num<0 || ui->mangaView->getPageCount()<=0)
        ui->lblPosition->clear();
    else
        ui->lblPosition->setText(tr("%1 / %2").arg(num+1).arg(ui->mangaView->getPageCount()));

    if (!msg.isEmpty())
        statusBar()->showMessage(msg);
    else
        statusBar()->clearMessage();
}

void MainWindow::switchFullscreen()
{
    fullScreen = !fullScreen;

    statusBar()->setVisible(!fullScreen);
    menuBar()->setVisible(!fullScreen);
    ui->tabWidget->tabBar()->setVisible(!fullScreen);
    ui->toolbar->setVisible(!fullScreen);
    if (fullScreen)
        showFullScreen();
    else
        showNormal();
    ui->actionFullscreen->setChecked(fullScreen);
}

void MainWindow::viewerKeyPressed(int key)
{
    if (key==Qt::Key_Escape && fullScreen) switchFullscreen();
    if (key==Qt::Key_F4) showMinimized();
}

void MainWindow::updateViewer()
{
    ui->mangaView->redrawPage();
}

void MainWindow::updateBookmarks()
{
    while (bookmarksMenu->actions().count()>2)
        bookmarksMenu->removeAction(bookmarksMenu->actions().last());
    foreach (const QString &t, zGlobal->bookmarks.keys()) {
        QAction* a = bookmarksMenu->addAction(t,this,SLOT(openBookmark()));
        a->setData(zGlobal->bookmarks.value(t));
        QString st = zGlobal->bookmarks.value(t);
        if (st.split('\n').count()>0)
            st = st.split('\n').at(0);
        a->setStatusTip(st);
    }
}

void MainWindow::updateTitle()
{
    QString t(tr("QManga"));
    if (!ui->mangaView->openedFile.isEmpty()) {
        QFileInfo fi(ui->mangaView->openedFile);
        t += tr(" - %1").arg(fi.fileName());
    } else {
        t += tr(" - Manga viewer and indexer");
    }
    setWindowTitle(t);
}

void MainWindow::createBookmark()
{
    if (ui->mangaView->openedFile.isEmpty()) return;
    QFileInfo fi(ui->mangaView->openedFile);
    QBookmarkDlg *dlg = new QBookmarkDlg(this,fi.completeBaseName(),ui->mangaView->openedFile);
    if (dlg->exec()) {
        QString t = dlg->getBkTitle();
        if (!t.isEmpty() && !zGlobal->bookmarks.contains(t)) {
            zGlobal->bookmarks[t]=QString("%1\n%2").arg(dlg->getBkFilename()).arg(ui->mangaView->currentPage);
            updateBookmarks();
        } else
            QMessageBox::warning(this,tr("QManga"),
                                 tr("Unable to add bookmark (frame is empty or duplicate title). Try again."));
    }
    dlg->setParent(NULL);
    delete dlg;
}

void MainWindow::openBookmark()
{
    QAction* a = qobject_cast<QAction *>(sender());
    if (a==NULL) return;

    QString f = a->data().toString();
    QStringList sl = a->data().toString().split('\n');
    int page = 0;
    if (sl.count()>1) {
        f = sl.at(0);
        bool okconv;
        page = sl.at(1).toInt(&okconv);
        if (!okconv) page = 0;
    }
    QFileInfo fi(f);
    if (!fi.isReadable()) return;

    ui->tabWidget->setCurrentIndex(0);
    ui->mangaView->openFile(f,page);
}

void MainWindow::helpAbout()
{
    QMessageBox::about(this, tr("QManga"),
                       tr("Manga reader with MySQL indexer.\n\nLicensed under GPL v3 license.\n\n"
                          "Author: kilobax.\nApp icon (Alien9) designer: EXO."));

}

void MainWindow::msgFromIndexer(QString msg)
{
    lblSearchStatus->setText(msg);
}
