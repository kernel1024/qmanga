#include <QDesktopWidget>
#include <QMessageBox>
#include <QCloseEvent>
#include <QStatusBar>
#include <QPushButton>
#include <QApplication>
#include <QCoreApplication>
#include <QClipboard>
#include <QImage>
#include <QBuffer>

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
    searchTab = ui->searchTab;
    fullScreen = false;

//    QApplication::setWheelScrollLines(1);

    lblSearchStatus = new QLabel(tr("Ready"));
    lblAverageSizes = new QLabel();
    lblRotation = new QLabel("Rotation: 0");
    statusBar()->addPermanentWidget(lblAverageSizes);
    statusBar()->addPermanentWidget(lblSearchStatus);
    statusBar()->addPermanentWidget(lblRotation);

    ui->spinPosition->hide();
    ui->fastScrollPanel->setMainWindow(this);
    ui->fastScrollSlider->hide();
    ui->fastScrollSlider->installEventFilter(this);

    ui->fsResults->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->actionExit,SIGNAL(triggered()),this,SLOT(close()));
    connect(ui->actionOpen,SIGNAL(triggered()),this,SLOT(openAux()));
    connect(ui->actionOpenClipboard,SIGNAL(triggered()),this,SLOT(openClipboard()));
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
    connect(searchTab,SIGNAL(mangaDblClick(QString)),this,SLOT(openFromIndex(QString)));
    connect(searchTab,SIGNAL(statusBarMsg(QString)),this,SLOT(msgFromIndexer(QString)));
    connect(ui->mangaView,SIGNAL(averageSizes(QSize,qint64)),this,SLOT(msgFromMangaView(QSize,qint64)));
    connect(ui->spinPosition,SIGNAL(editingFinished()),this,SLOT(pageNumEdited()));
    connect(ui->btnRotateCCW,SIGNAL(clicked()),ui->mangaView,SLOT(viewRotateCCW()));
    connect(ui->btnRotateCW,SIGNAL(clicked()),ui->mangaView,SLOT(viewRotateCW()));
    connect(ui->mangaView,SIGNAL(rotationUpdated(int)),this,SLOT(rotationUpdated(int)));

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

    connect(ui->fastScrollSlider,SIGNAL(valueChanged(int)),this,SLOT(fastScroll(int)));
    connect(ui->fastScrollPanel,SIGNAL(showWidget()),this,SLOT(updateFastScrollPosition()));

    connect(ui->fsResults,SIGNAL(customContextMenuRequested(QPoint)),
            this,SLOT(fsResultsMenuCtx(QPoint)));
    connect(ui->btnFSAdd,SIGNAL(clicked()),this,SLOT(fsAddFiles()));
    connect(ui->btnFSCheck,SIGNAL(clicked()),this,SLOT(fsCheckAvailability()));
    connect(ui->btnFSDelete,SIGNAL(clicked()),this,SLOT(fsDelFiles()));
    connect(zg,SIGNAL(fsFilesAdded()),this,SLOT(fsNewFilesAdded()));
    connect(this,SIGNAL(dbAddFiles(QStringList,QString)),
            zg->db,SLOT(sqlAddFiles(QStringList,QString)),Qt::QueuedConnection);

    ui->mangaView->scroller = ui->scrollArea;
    zg->loadSettings();
    if (!isMaximized())
        centerWindow();
    savedPos=pos();
    savedSize=size();
    savedMaximized=isMaximized();
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
    searchTab->updateSplitters();
}

bool MainWindow::isMangaOpened()
{
    return (ui->mangaView->getPageCount()>0);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (zg!=NULL)
        zg->saveSettings();
    event->accept();
}

void MainWindow::resizeEvent(QResizeEvent *)
{
    zg->resetPreferredWidth();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj==ui->fastScrollSlider && event->type()==QEvent::Wheel) {
        QCoreApplication::sendEvent(ui->mangaView,event);
        return true;
    } else
        return QMainWindow::eventFilter(obj,event);
}

void MainWindow::openAux()
{
    QString filename = getOpenFileNameD(this,tr("Open manga or image"),zg->savedAuxOpenDir);
    if (!filename.isEmpty()) {
        QFileInfo fi(filename);
        zg->savedAuxOpenDir = fi.path();

        ui->tabWidget->setCurrentIndex(0);
        ui->mangaView->openFile(filename);
    }
}

void MainWindow::openClipboard()
{
    if (!QApplication::clipboard()->mimeData()->hasImage()) {
        QMessageBox::warning(this,tr("QManga"),tr("Clipboard is empty or contains incompatible data."));
        return;
    }

    QImage img = QApplication::clipboard()->image();
    QByteArray imgs;
    imgs.clear();
    QBuffer buf(&imgs);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf,"BMP");
    buf.close();
    QString bs = QString(":CLIP:")+QString::fromLatin1(imgs.toBase64());
    imgs.clear();

    ui->tabWidget->setCurrentIndex(0);
    ui->mangaView->openFile(bs);
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
    if (num<0 || !isMangaOpened()) {
        if (ui->spinPosition->isVisible()) {
            ui->spinPosition->hide();
        }
        ui->lblPosition->clear();
        ui->fastScrollSlider->setEnabled(false);
    } else {
        if (!ui->spinPosition->isVisible()) {
            ui->spinPosition->show();
            ui->spinPosition->setMinimum(1);
            ui->spinPosition->setMaximum(ui->mangaView->getPageCount());
        }
        ui->spinPosition->setValue(num+1);
        ui->lblPosition->setText(tr("  /  %1").arg(ui->mangaView->getPageCount()));

        ui->fastScrollSlider->blockSignals(true);
        ui->fastScrollSlider->setEnabled(true);
        ui->fastScrollSlider->setMinimum(1);
        ui->fastScrollSlider->setMaximum(ui->mangaView->getPageCount());
        if (ui->fastScrollSlider->value()!=num)
            ui->fastScrollSlider->setValue(num+1);
        ui->fastScrollSlider->blockSignals(false);
    }

    if (!msg.isEmpty())
        statusBar()->showMessage(msg);
    else
        statusBar()->clearMessage();
}

void MainWindow::switchFullscreen()
{
    fullScreen = !fullScreen;

    if (fullScreen) {
        savedMaximized=isMaximized();
        savedPos=pos();
        savedSize=size();
    }

    statusBar()->setVisible(!fullScreen);
    menuBar()->setVisible(!fullScreen);
    ui->tabWidget->tabBar()->setVisible(!fullScreen);
    ui->toolbar->setVisible(!fullScreen);
    int m = 2;
    if (fullScreen) m = 0;
    if (ui->centralWidget!=NULL) {
        ui->centralWidget->layout()->setMargin(m);
        ui->centralWidget->layout()->setContentsMargins(m,m,m,m);
    }
    if (ui->tabWidget->currentWidget()!=NULL) {
        ui->tabWidget->currentWidget()->layout()->setMargin(m);
        ui->tabWidget->currentWidget()->layout()->setContentsMargins(m,m,m,m);
    }
    if (fullScreen)
        showFullScreen();
    else {
        if (savedMaximized)
            showMaximized();
        else {
            showNormal();
            move(savedPos);
            resize(savedSize);
        }
    }
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

void MainWindow::rotationUpdated(int degree)
{
    lblRotation->setText(tr("Rotation: %1").arg(degree));
}

void MainWindow::fastScroll(int page)
{
    if (!isMangaOpened()) return;
    if (page<=0 || page>ui->mangaView->getPageCount()) return;
    ui->mangaView->setPage(page-1);
}

void MainWindow::updateFastScrollPosition()
{
    if (!isMangaOpened()) return;
    ui->fastScrollSlider->blockSignals(true);
    ui->fastScrollSlider->setValue(ui->mangaView->currentPage+1);
    ui->fastScrollSlider->blockSignals(false);
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

void MainWindow::fsAddFiles()
{
    QHash<QString,QStringList> fl;
    fl.clear();
    for (int i=0;i<fsScannedFiles.count();i++) {
        fl[fsScannedFiles.at(i).album].append(fsScannedFiles.at(i).fileName);
        zg->newlyAddedFiles.removeAll(fsScannedFiles.at(i).fileName);
    }

    for (int i=0;i<fl.keys().count();i++) {
        QString k = fl.keys().at(i);
        emit dbAddFiles(fl[k],k);
    }

    zg->fsCheckFilesAvailability();
}

void MainWindow::fsCheckAvailability()
{
    zg->fsCheckFilesAvailability();
}

void MainWindow::fsDelFiles()
{
    QList<int> rows;
    rows.clear();
    for (int i=0;i<ui->fsResults->selectedItems().count();i++) {
        int idx = ui->fsResults->selectedItems().at(i)->row();
        if (!rows.contains(idx))
            rows << idx;
    }
    if (rows.isEmpty()) return;
    QList<ZFSFile> nl;
    nl.clear();
    for (int i=0;i<fsScannedFiles.count();i++) {
        if (!rows.contains(i))
            nl << fsScannedFiles.at(i);
    }
    fsScannedFiles = nl;
    fsUpdateFileList();
}

void MainWindow::fsNewFilesAdded()
{
    fsAddFilesMutex.lock();
    ui->fsResults->clear();
    fsScannedFiles.clear();
    QStringList f = zg->newlyAddedFiles;

    for (int i=0;i<f.count();i++) {
        QFileInfo fi(f.at(i));
        fsScannedFiles << ZFSFile(fi.fileName(),fi.absoluteFilePath(),fi.absoluteDir().dirName());
    }
    fsUpdateFileList();
    fsAddFilesMutex.unlock();
}

void MainWindow::fsUpdateFileList()
{
    QStringList h;
    h << tr("File") << tr("Album");
    ui->fsResults->setColumnCount(2);
    ui->fsResults->setRowCount(fsScannedFiles.count());
    ui->fsResults->setHorizontalHeaderLabels(h);
    for (int i=0;i<fsScannedFiles.count();i++) {
        ui->fsResults->setItem(i,0,new QTableWidgetItem(fsScannedFiles[i].name));
        ui->fsResults->setItem(i,1,new QTableWidgetItem(fsScannedFiles[i].album));
    }
    ui->fsResults->setColumnWidth(0,ui->tabWidget->width()/2);
}

void MainWindow::fsResultsMenuCtx(const QPoint &pos)
{
    QStringList albums = searchTab->getAlbums();
    if (albums.isEmpty()) return;
    QMenu cm(this);
    for (int i=0;i<albums.count();i++) {
        QAction* ac = new QAction(albums.at(i),NULL);
        connect(ac,SIGNAL(triggered()),this,SLOT(fsResultsCtxApplyAlbum()));
        cm.addAction(ac);
    }
    cm.exec(ui->fsResults->mapToGlobal(pos));
}

void MainWindow::fsResultsCtxApplyAlbum()
{
    QAction* ac = qobject_cast<QAction *>(sender());
    if (ac==NULL) return;
    QString s = ac->text();
    QList<int> idxs;
    idxs.clear();

    for (int i=0;i<ui->fsResults->selectedItems().count();i++)
        idxs << ui->fsResults->selectedItems().at(i)->row();

    for (int i=0;i<idxs.count();i++)
        if (ui->fsResults->item(idxs.at(i),1)!=NULL)
            ui->fsResults->item(idxs.at(i),1)->setText(s);

}

void MainWindow::pageNumEdited()
{
    if (ui->spinPosition->value()!=ui->mangaView->currentPage)
        ui->mangaView->setPage(ui->spinPosition->value()-1);
}


ZFSFile::ZFSFile()
{
    name.clear();
    fileName.clear();
    album.clear();
}

ZFSFile::ZFSFile(QString aName, QString aFileName, QString aAlbum)
{
    name = aName;
    fileName = aFileName;
    album = aAlbum;
}

ZFSFile &ZFSFile::operator=(const ZFSFile &other)
{
    name = other.name;
    fileName = other.fileName;
    album = other.album;
    return *this;
}


ZPopupFrame::ZPopupFrame(QWidget *parent) :
    QFrame(parent)
{
    mwnd = NULL;
    hideChildren();
}

void ZPopupFrame::setMainWindow(MainWindow *wnd)
{
    mwnd = wnd;
}

void ZPopupFrame::enterEvent(QEvent *)
{
    showChildren();
}

void ZPopupFrame::leaveEvent(QEvent *)
{
    hideChildren();
}

void ZPopupFrame::hideChildren()
{
    foreach (QWidget* w, findChildren<QWidget *>())
        if (w!=NULL) w->hide();
    emit hideWidget();
}

void ZPopupFrame::showChildren()
{
    if (mwnd!=NULL && !mwnd->isMangaOpened())
        hideChildren();
    else {
        foreach (QWidget* w, findChildren<QWidget *>())
            if (w!=NULL) w->show();
        emit showWidget();
    }
}
