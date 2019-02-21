#include <QDesktopWidget>
#include <QMessageBox>
#include <QCloseEvent>
#include <QStatusBar>
#include <QPushButton>
#include <QApplication>
#include <QCoreApplication>
#include <QClipboard>
#include <QMimeData>
#include <QImage>
#include <QBuffer>
#include <QScreen>
#include <cmath>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "zglobal.h"
#include "bookmarkdlg.h"
#include "ocreditor.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    if (zg==nullptr)
        zg = new ZGlobal(this);

    ui->setupUi(this);

    if (zg->ocrEditor==nullptr)
        zg->ocrEditor = new ZOCREditor(this);

    setWindowIcon(QIcon(":/Alien9"));

    bookmarksMenu = ui->menuBookmarks;
    searchTab = ui->searchTab;
    fullScreen = false;

    indexerMsgBox.setWindowTitle(tr("QManga"));

    lblSearchStatus = new QLabel(tr("Ready"));
    lblAverageSizes = new QLabel();
    lblRotation = new QLabel("Rotation: 0");
    lblCrop = new QLabel();

    statusBar()->addPermanentWidget(lblAverageSizes);
    statusBar()->addPermanentWidget(lblSearchStatus);
    statusBar()->addPermanentWidget(lblRotation);
    statusBar()->addPermanentWidget(lblCrop);

    ui->spinPosition->hide();
    ui->fastScrollPanel->setMainWindow(this);
    ui->fastScrollSlider->hide();
    ui->fastScrollSlider->installEventFilter(this);
    ui->mouseModeOCR->hide();
    ui->mouseModePan->hide();
    ui->mouseModeCrop->hide();

    ui->mouseModeOCR->setChecked(true);

    ui->fsResults->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->actionExit,&QAction::triggered,this,&MainWindow::close);
    connect(ui->actionOpen,&QAction::triggered,this,&MainWindow::openAux);
    connect(ui->actionOpenClipboard,&QAction::triggered,this,&MainWindow::openClipboard);
    connect(ui->actionClose,&QAction::triggered,this,&MainWindow::closeManga);
    connect(ui->actionSettings,&QAction::triggered,zg,&ZGlobal::settingsDlg);
    connect(ui->actionAddBookmark,&QAction::triggered,this,&MainWindow::createBookmark);
    connect(ui->actionAbout,&QAction::triggered,this,&MainWindow::helpAbout);
    connect(ui->actionFullscreen,&QAction::triggered,this,&MainWindow::switchFullscreen);
    connect(ui->actionMinimize,&QAction::triggered,this,&MainWindow::showMinimized);
    connect(ui->actionSaveSettings,&QAction::triggered,zg,&ZGlobal::saveSettings);
    connect(ui->actionShowOCR,&QAction::triggered,zg->ocrEditor,&ZOCREditor::show);

    connect(ui->btnOpen,&QPushButton::clicked,this,&MainWindow::openAux);
    connect(ui->mangaView,&ZMangaView::loadedPage,this,&MainWindow::dispPage);
    connect(ui->scrollArea,&ZScrollArea::sizeChanged,ui->mangaView,&ZMangaView::ownerResized);
    connect(ui->comboZoom,qOverload<const QString &>(&QComboBox::currentIndexChanged),
            ui->mangaView,&ZMangaView::setZoomAny);
    connect(ui->mangaView,&ZMangaView::doubleClicked,this,&MainWindow::switchFullscreen);
    connect(ui->mangaView,&ZMangaView::keyPressed,this,&MainWindow::viewerKeyPressed);
    connect(ui->mangaView,&ZMangaView::minimizeRequested,this,&MainWindow::showMinimized);
    connect(ui->mangaView,&ZMangaView::closeFileRequested,this,&MainWindow::closeManga);
    connect(searchTab,&ZSearchTab::mangaDblClick,this,&MainWindow::openFromIndex);
    connect(searchTab,&ZSearchTab::statusBarMsg,this,&MainWindow::auxMessage);
    connect(ui->mangaView,&ZMangaView::auxMessage,this,&MainWindow::auxMessage);
    connect(ui->mangaView,&ZMangaView::averageSizes,this,&MainWindow::msgFromMangaView);
    connect(ui->spinPosition,&QSpinBox::editingFinished,this,&MainWindow::pageNumEdited);
    connect(ui->btnRotateCCW,&QPushButton::clicked,ui->mangaView,&ZMangaView::viewRotateCCW);
    connect(ui->btnRotateCW,&QPushButton::clicked,ui->mangaView,&ZMangaView::viewRotateCW);
    connect(ui->mangaView,&ZMangaView::rotationUpdated,this,&MainWindow::rotationUpdated);
    connect(ui->mangaView,&ZMangaView::cropUpdated,this,&MainWindow::cropUpdated);

    connect(ui->btnNavFirst,&QPushButton::clicked,ui->mangaView,&ZMangaView::navFirst);
    connect(ui->btnNavPrev,&QPushButton::clicked,ui->mangaView,&ZMangaView::navPrev);
    connect(ui->btnNavNext,&QPushButton::clicked,ui->mangaView,&ZMangaView::navNext);
    connect(ui->btnNavLast,&QPushButton::clicked,ui->mangaView,&ZMangaView::navLast);

    connect(ui->btnZoomFit,&QPushButton::clicked,ui->mangaView,&ZMangaView::setZoomFit);
    connect(ui->btnZoomHeight,&QPushButton::clicked,ui->mangaView,&ZMangaView::setZoomHeight);
    connect(ui->btnZoomWidth,&QPushButton::clicked,ui->mangaView,&ZMangaView::setZoomWidth);
    connect(ui->btnZoomOriginal,&QPushButton::clicked,ui->mangaView,&ZMangaView::setZoomOriginal);
    connect(ui->btnZoomDynamic,&QPushButton::toggled,ui->mangaView,&ZMangaView::setZoomDynamic);

    connect(ui->btnSearchTab,&QPushButton::clicked,this,&MainWindow::openSearchTab);
    connect(ui->tabWidget,&QTabWidget::currentChanged,this,&MainWindow::tabChanged);

    connect(ui->fastScrollSlider,&QSlider::valueChanged,this,&MainWindow::fastScroll);
    connect(ui->fastScrollPanel,&ZPopupFrame::showWidget,this,&MainWindow::updateFastScrollPosition);

    connect(ui->mouseModeOCR,&QRadioButton::toggled,this,&MainWindow::changeMouseMode);
    connect(ui->mouseModePan,&QRadioButton::toggled,this,&MainWindow::changeMouseMode);
    connect(ui->mouseModeCrop,&QRadioButton::toggled,this,&MainWindow::changeMouseMode);

    connect(ui->fsResults,&QTableWidget::customContextMenuRequested,
            this,&MainWindow::fsResultsMenuCtx);
    connect(ui->btnFSAdd,&QPushButton::clicked,this,&MainWindow::fsAddFiles);
    connect(ui->btnFSCheck,&QPushButton::clicked,this,&MainWindow::fsCheckAvailability);
    connect(ui->btnFSDelete,&QPushButton::clicked,this,&MainWindow::fsDelFiles);
    connect(ui->btnFSFind,&QPushButton::clicked,this,&MainWindow::fsFindNewFiles);
    connect(zg->db,&ZDB::foundNewFiles,
            this,&MainWindow::fsFoundNewFiles,Qt::QueuedConnection);
    connect(this,&MainWindow::dbAddIgnoredFiles,
            zg->db,&ZDB::sqlAddIgnoredFiles,Qt::QueuedConnection);
    connect(this,&MainWindow::dbFindNewFiles,
            zg->db,&ZDB::sqlSearchMissingManga,Qt::QueuedConnection);
    connect(zg,&ZGlobal::fsFilesAdded,this,&MainWindow::fsNewFilesAdded);
    connect(this,&MainWindow::dbAddFiles,
            zg->db,&ZDB::sqlAddFiles,Qt::QueuedConnection);

    ui->mangaView->scroller = ui->scrollArea;
    zg->loadSettings();
    centerWindow(!isMaximized());
    savedPos=pos();
    savedSize=size();
    savedMaximized=isMaximized();
    dispPage(-1,QString());

    QString fname;
    if (QApplication::arguments().count()>1)
        fname = QApplication::arguments().at(1);
    if (!fname.isEmpty() && !fname.startsWith("--"))
        openAuxFile(fname);
}

MainWindow::~MainWindow()
{
    delete ui;
    zg = nullptr;
}

void MainWindow::centerWindow(bool moveWindow)
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

    if (moveWindow) {
        resize(nw);
        move(rect.width()/2 - frameGeometry().width()/2,
             rect.height()/2 - frameGeometry().height()/2);
        searchTab->updateSplitters();
    }

    zg->dpiX = QApplication::screens().at(screen)->physicalDotsPerInchX();
    zg->dpiY = QApplication::screens().at(screen)->physicalDotsPerInchY();
#ifdef Q_OS_WIN
    if (abs(zg->dpiX-zg->dpiY)>20.0) {
        qreal dpi = qMax(zg->dpiX,zg->dpiY);
        if (dpi<130.0) dpi=130.0;
        zg->dpiX = dpi;
        zg->dpiY = dpi;
    }
#endif
}

bool MainWindow::isMangaOpened()
{
    return (ui->mangaView->getPageCount()>0);
}

void MainWindow::openAuxFile(const QString &filename)
{
    QFileInfo fi(filename);
    zg->savedAuxOpenDir = fi.path();

    ui->tabWidget->setCurrentIndex(0);
    ui->mangaView->openFile(filename);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (zg!=nullptr)
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
    }

    return QMainWindow::eventFilter(obj,event);
}

void MainWindow::openAux()
{
    QString filename = getOpenFileNameD(this,tr("Open manga or image"),zg->savedAuxOpenDir);
    if (!filename.isEmpty())
        openAuxFile(filename);
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

void MainWindow::openFromIndex(const QString &filename)
{
    ui->tabWidget->setCurrentIndex(0);
    ui->mangaView->openFile(filename);
}

void MainWindow::closeManga()
{
    ui->mangaView->closeFile();
    ui->spinPosition->hide();
}

void MainWindow::dispPage(int num, const QString &msg)
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
    if (ui->centralWidget!=nullptr) {
        ui->centralWidget->layout()->setMargin(m);
        ui->centralWidget->layout()->setContentsMargins(m,m,m,m);
    }
    if (ui->tabWidget->currentWidget()!=nullptr) {
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

void MainWindow::cropUpdated(const QRect &crop)
{
    if (!crop.isNull())
        lblCrop->setText(tr("Cropped: %1x%2").arg(crop.width()).arg(crop.height()));
    else
        lblCrop->clear();
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

void MainWindow::tabChanged(int idx)
{
    if (idx==1 && searchTab!=nullptr)
        searchTab->updateFocus();
}

void MainWindow::changeMouseMode(bool state)
{
    Q_UNUSED(state)

    if (ui->mouseModeOCR->isChecked())
        ui->mangaView->setMouseMode(ZMangaView::mmOCR);
    else if (ui->mouseModePan->isChecked())
        ui->mangaView->setMouseMode(ZMangaView::mmPan);
    else if (ui->mouseModeCrop->isChecked())
        ui->mangaView->setMouseMode(ZMangaView::mmCrop);
}

void MainWindow::updateBookmarks()
{
    while (bookmarksMenu->actions().count()>2)
        bookmarksMenu->removeAction(bookmarksMenu->actions().constLast());
    for (auto it = zg->bookmarks.keyValueBegin(), end = zg->bookmarks.keyValueEnd();
         it != end; ++it) {
        QAction* a = bookmarksMenu->addAction((*it).first,this,&MainWindow::openBookmark);
        QString st = (*it).second;
        a->setData(st);
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
    QTwoEditDlg *dlg = new QTwoEditDlg(this,tr("Add bookmark"),tr("Title"),tr("Filename"),
                                       fi.completeBaseName(),ui->mangaView->openedFile);
    if (dlg->exec()) {
        QString t = dlg->getDlgEdit1();
        if (!t.isEmpty() && !zg->bookmarks.contains(t)) {
            zg->bookmarks[t]=QString("%1\n%2").arg(dlg->getDlgEdit2()).arg(ui->mangaView->currentPage);
            updateBookmarks();
        } else
            QMessageBox::warning(this,tr("QManga"),
                                 tr("Unable to add bookmark (frame is empty or duplicate title). Try again."));
    }
    dlg->setParent(nullptr);
    delete dlg;
}

void MainWindow::openBookmark()
{
    auto a = qobject_cast<QAction *>(sender());
    if (a==nullptr) return;

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
    if (!fi.isReadable()) {
        QMessageBox::warning(this,tr("QManga"),tr("Unable to open file %1").arg(f));
        return;
    }

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
                          "Author: kernelonline.\n"
                          "App icon (Alien9) designer: EXO.\n"
                          "Icon pack: KDE team Oxygen project."));

}

void MainWindow::auxMessage(const QString &msg)
{
    QString s = msg;
    bool showMsgBox = s.startsWith("MBOX#");
    if (showMsgBox)
        s.remove("MBOX#");
    lblSearchStatus->setText(s);
    if (showMsgBox) {
        if (indexerMsgBox.isVisible())
            indexerMsgBox.setText(indexerMsgBox.text()+"\n"+s);
        else
            indexerMsgBox.setText(s);
        indexerMsgBox.exec();
    }
}

void MainWindow::msgFromMangaView(const QSize &sz, qint64 fsz)
{
    lblAverageSizes->setText(tr("Avg: %1x%2, %3").arg(sz.width()).arg(sz.height()).arg(formatSize(fsz)));
}

void MainWindow::fsAddFiles()
{
    QHash<QString,QStringList> fl;
    fl.clear();
    for (const auto &i : qAsConst(fsScannedFiles)) {
        fl[i.album].append(i.fileName);
        zg->newlyAddedFiles.removeAll(i.fileName);
    }

    for (auto it = fl.keyValueBegin(), end = fl.keyValueEnd(); it != end; ++it)
        emit dbAddFiles((*it).second,(*it).first);

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
    const QList<QTableWidgetItem *> list = ui->fsResults->selectedItems();
    for (const auto &i : list) {
        int idx = i->row();
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
    const QStringList f = zg->newlyAddedFiles;

    for (const auto &i : f) {
        QFileInfo fi(i);
        bool mimeOk;
        readerFactory(this,fi.absoluteFilePath(),&mimeOk,true,false);
        if (mimeOk)
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
        QTableWidgetItem* w = ui->fsResults->item(i,0);
        if (w!=nullptr)
                w->setToolTip(fsScannedFiles[i].fileName);
    }
    ui->fsResults->setColumnWidth(0,ui->tabWidget->width()/2);
}

void MainWindow::fsResultsMenuCtx(const QPoint &pos)
{
    QStringList albums = searchTab->getAlbums();
    QMenu cm(this);
    QAction* ac;
    int cnt=0;
    if (!ui->fsResults->selectedItems().isEmpty()) {
        ac = new QAction(QIcon(":/16/dialog-cancel"),tr("Ignore these file(s)"),this);
        connect(ac,&QAction::triggered,this,&MainWindow::fsAddIgnoredFiles);
        cm.addAction(ac);
        cm.addSeparator();
        cnt++;
    }
    for (const auto &i : albums) {
        if (i.startsWith("#")) continue;
        ac = new QAction(i,nullptr);
        connect(ac,&QAction::triggered,this,&MainWindow::fsResultsCtxApplyAlbum);
        ac->setData(i);
        cm.addAction(ac);
        cnt++;
    }
    if (cnt>0)
        cm.exec(ui->fsResults->mapToGlobal(pos));
}

void MainWindow::fsResultsCtxApplyAlbum()
{
    auto ac = qobject_cast<QAction *>(sender());
    if (ac==nullptr) return;
    QString s = ac->data().toString();
    if (s.isEmpty()) {
        QMessageBox::warning(this,tr("QManga"),tr("Unable to apply album name"));
        return;
    }
    QList<int> idxs;
    idxs.clear();

    const QList<QTableWidgetItem *> list = ui->fsResults->selectedItems();
    for (const auto &i : list)
        idxs << i->row();

    for (const auto &i : qAsConst(idxs))
        fsScannedFiles[i].album = s;

    fsUpdateFileList();
}

void MainWindow::fsFindNewFiles()
{
    fsScannedFiles.clear();
    fsUpdateFileList();
    searchTab->dbShowProgressDialogEx(true,tr("Scanning filesystem"));
    searchTab->dbShowProgressState(25,tr("Scanning filesystem..."));
    emit dbFindNewFiles();
}

void MainWindow::fsFoundNewFiles(const QStringList &files)
{
    for (const QString& filename : files) {
        QFileInfo fi(filename);
        bool mimeOk;
        readerFactory(this,fi.absoluteFilePath(),&mimeOk,true,false);
        if (mimeOk)
            fsScannedFiles << ZFSFile(fi.fileName(),fi.absoluteFilePath(),fi.absoluteDir().dirName());
    }
    fsUpdateFileList();
    searchTab->dbShowProgressDialog(false);
}

void MainWindow::fsAddIgnoredFiles()
{
    QList<int> rows;
    rows.clear();
    const QList<QTableWidgetItem *> list = ui->fsResults->selectedItems();
    for (const auto &i : list) {
        int idx = i->row();
        if (!rows.contains(idx))
            rows << idx;
    }
    if (rows.isEmpty()) return;
    QList<ZFSFile> nl;
    QStringList sl;
    nl.clear();
    sl.clear();
    for (int i=0;i<fsScannedFiles.count();i++) {
        if (!rows.contains(i))
            nl << fsScannedFiles.at(i);
        else
            sl << fsScannedFiles.at(i).fileName;

    }
    emit dbAddIgnoredFiles(sl);
    fsScannedFiles = nl;
    fsUpdateFileList();
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

ZFSFile::ZFSFile(const ZFSFile &other)
{
    name = other.name;
    fileName = other.fileName;
    album = other.album;
}

ZFSFile::ZFSFile(const QString &aName, const QString &aFileName, const QString &aAlbum)
{
    name = aName;
    fileName = aFileName;
    album = aAlbum;
}

ZPopupFrame::ZPopupFrame(QWidget *parent) :
    QFrame(parent)
{
    mwnd = nullptr;
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
    const QList<QWidget *> list = findChildren<QWidget *>();
    for (QWidget* w : list)
        if (w!=nullptr) w->hide();
    emit hideWidget();
}

void ZPopupFrame::showChildren()
{
    if (mwnd!=nullptr && !mwnd->isMangaOpened())
        hideChildren();
    else {
        const QList<QWidget *> list = findChildren<QWidget *>();
        for (QWidget* w : list)
            if (w!=nullptr) w->show();
        emit showWidget();
    }
}
