#include <QDesktopWidget>
#include <QMessageBox>
#include <QCloseEvent>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "zglobal.h"
#include "bookmarkdlg.h"
#include "ocreditor.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    if (zg==NULL)
        zg = new ZGlobal(this);

    ui->setupUi(this);

    if (zg->ocrEditor==NULL)
        zg->ocrEditor = new ZOCREditor(this);

    setWindowIcon(QIcon(":/img/Alien9.png"));

    bookmarksMenu = ui->menuBookmarks;
    srcWidget = ui->srcWidget;
    fullScreen = false;

    QApplication::setWheelScrollLines(1);

    lblSearchStatus = new QLabel(tr("Ready"));
    lblAverageSizes = new QLabel();
    statusBar()->addPermanentWidget(lblAverageSizes);
    statusBar()->addPermanentWidget(lblSearchStatus);

    ui->spinPosition->hide();

    connect(ui->actionExit,SIGNAL(triggered()),this,SLOT(close()));
    connect(ui->actionOpen,SIGNAL(triggered()),this,SLOT(openAux()));
    connect(ui->actionClose,SIGNAL(triggered()),this,SLOT(closeManga()));
    connect(ui->actionSettings,SIGNAL(triggered()),zg,SLOT(settingsDlg()));
    connect(ui->actionAddBookmark,SIGNAL(triggered()),this,SLOT(createBookmark()));
    connect(ui->actionAbout,SIGNAL(triggered()),this,SLOT(helpAbout()));
    connect(ui->actionFullscreen,SIGNAL(triggered()),this,SLOT(switchFullscreen()));
    connect(ui->actionMinimize,SIGNAL(triggered()),this,SLOT(showMinimized()));
    connect(ui->actionSaveSettings,SIGNAL(triggered()),zg,SLOT(saveSettings()));

    connect(ui->btnOpen,SIGNAL(clicked()),this,SLOT(openAux()));
    connect(ui->mangaView,SIGNAL(loadedPage(int,QString)),this,SLOT(dispPage(int,QString)));
    connect(ui->scrollArea,SIGNAL(sizeChanged(QSize)),ui->mangaView,SLOT(ownerResized(QSize)));
    connect(ui->comboZoom,SIGNAL(currentIndexChanged(QString)),ui->mangaView,SLOT(setZoomAny(QString)));
    connect(ui->mangaView,SIGNAL(doubleClicked()),this,SLOT(switchFullscreen()));
    connect(ui->mangaView,SIGNAL(keyPressed(int)),this,SLOT(viewerKeyPressed(int)));
    connect(ui->mangaView,SIGNAL(minimizeRequested()),this,SLOT(showMinimized()));
    connect(ui->mangaView,SIGNAL(closeFileRequested()),this,SLOT(closeManga()));
    connect(ui->srcWidget,SIGNAL(mangaDblClick(QString)),this,SLOT(openFromIndex(QString)));
    connect(ui->srcWidget,SIGNAL(statusBarMsg(QString)),this,SLOT(msgFromIndexer(QString)));
    connect(ui->mangaView,SIGNAL(averageSizes(QSize,qint64)),this,SLOT(msgFromMangaView(QSize,qint64)));
    connect(ui->spinPosition,SIGNAL(editingFinished()),this,SLOT(pageNumEdited()));

    connect(ui->btnNavFirst,SIGNAL(clicked()),ui->mangaView,SLOT(navFirst()));
    connect(ui->btnNavPrev,SIGNAL(clicked()),ui->mangaView,SLOT(navPrev()));
    connect(ui->btnNavNext,SIGNAL(clicked()),ui->mangaView,SLOT(navNext()));
    connect(ui->btnNavLast,SIGNAL(clicked()),ui->mangaView,SLOT(navLast()));

    connect(ui->btnZoomFit,SIGNAL(clicked()),ui->mangaView,SLOT(setZoomFit()));
    connect(ui->btnZoomHeight,SIGNAL(clicked()),ui->mangaView,SLOT(setZoomHeight()));
    connect(ui->btnZoomWidth,SIGNAL(clicked()),ui->mangaView,SLOT(setZoomWidth()));
    connect(ui->btnZoomOriginal,SIGNAL(clicked()),ui->mangaView,SLOT(setZoomOriginal()));
    connect(ui->btnZoomDynamic,SIGNAL(toggled(bool)),ui->mangaView,SLOT(setZoomDynamic(bool)));

    connect(ui->btnSearchTab,SIGNAL(clicked()),this,SLOT(openSearchTab()));

    ui->mangaView->scroller = ui->scrollArea;
    zg->loadSettings();
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
    if (zg!=NULL)
        zg->saveSettings();
    event->accept();
}

void MainWindow::openAux()
{
    QString filename = getOpenFileNameD(this,tr("Open manga archive"),zg->savedAuxOpenDir);
    if (!filename.isEmpty()) {
        QFileInfo fi(filename);
        zg->savedAuxOpenDir = fi.path();

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
    ui->spinPosition->hide();
}

void MainWindow::dispPage(int num, QString msg)
{
    updateTitle();
    if (num<0 || ui->mangaView->getPageCount()<=0) {
        if (ui->spinPosition->isVisible()) {
            ui->spinPosition->hide();
        }
        ui->lblPosition->clear();
    } else {
        if (!ui->spinPosition->isVisible()) {
            ui->spinPosition->show();
            ui->spinPosition->setMinimum(1);
            ui->spinPosition->setMaximum(ui->mangaView->getPageCount());
        }
        ui->spinPosition->setValue(num+1);
        ui->lblPosition->setText(tr("  /  %1").arg(ui->mangaView->getPageCount()));
    }

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
}

void MainWindow::updateViewer()
{
    ui->mangaView->redrawPage();
}

void MainWindow::updateBookmarks()
{
    while (bookmarksMenu->actions().count()>2)
        bookmarksMenu->removeAction(bookmarksMenu->actions().last());
    foreach (const QString &t, zg->bookmarks.keys()) {
        QAction* a = bookmarksMenu->addAction(t,this,SLOT(openBookmark()));
        a->setData(zg->bookmarks.value(t));
        QString st = zg->bookmarks.value(t);
        if (st.split('\n').count()>0)
            st = st.split('\n').at(0);
        a->setStatusTip(st);
    }
}

void MainWindow::updateTitle()
{
    QString t;
    t.clear();
    if (!ui->mangaView->openedFile.isEmpty()) {
        QFileInfo fi(ui->mangaView->openedFile);
        t = tr("%1 - QManga").arg(fi.fileName());
    } else {
        t = tr("QManga - Manga viewer and indexer");
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
        if (!t.isEmpty() && !zg->bookmarks.contains(t)) {
            zg->bookmarks[t]=QString("%1\n%2").arg(dlg->getBkFilename()).arg(ui->mangaView->currentPage);
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

void MainWindow::openSearchTab()
{
    ui->tabWidget->setCurrentIndex(1);
}

void MainWindow::helpAbout()
{
    QMessageBox::about(this, tr("QManga"),
                       tr("Manga reader with MySQL indexer.\n\nLicensed under GPL v3 license.\n\n"
                          "Author: kernelonline.\nApp icon (Alien9) designer: EXO."));

}

void MainWindow::msgFromIndexer(QString msg)
{
    lblSearchStatus->setText(msg);
}

void MainWindow::msgFromMangaView(QSize sz, qint64 fsz)
{
    lblAverageSizes->setText(tr("Avg: %1x%2, %3").arg(sz.width()).arg(sz.height()).arg(formatSize(fsz)));
}

void MainWindow::pageNumEdited()
{
    if (ui->spinPosition->value()!=ui->mangaView->currentPage)
        ui->mangaView->setPage(ui->spinPosition->value()-1);
}
