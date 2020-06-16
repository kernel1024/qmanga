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
#include <QWindow>
#include <QKeySequence>
#include <QDebug>
#include <cmath>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "zglobal.h"
#include "bookmarkdlg.h"

ZMainWindow::ZMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ZMainWindow)
{
    ui->setupUi(this);
    zF->global()->setMainWindow(this);

    setWindowIcon(QIcon(QSL(":/Alien9")));

    ui->menuBookmarks->setStyleSheet(QSL("QMenu { menu-scrollable: 1; }"));

    m_indexerMsgBox.setWindowTitle(QGuiApplication::applicationDisplayName());

    m_lblSearchStatus = new QLabel(tr("Ready"));
    m_lblAverageSizes = new QLabel();
    m_lblRotation = new QLabel();
    m_lblCrop = new QLabel();
    m_lblAverageFineRender = new QLabel();
    rotationUpdated(0.0);

    statusBar()->addPermanentWidget(m_lblAverageFineRender);
    statusBar()->addPermanentWidget(m_lblAverageSizes);
    statusBar()->addPermanentWidget(m_lblSearchStatus);
    statusBar()->addPermanentWidget(m_lblRotation);
    statusBar()->addPermanentWidget(m_lblCrop);

    ui->spinPosition->hide();
    ui->fastScrollPanel->hide();
    ui->fastScrollSlider->setAutoFillBackground(true);

    ui->comboMouseMode->setCurrentIndex(0);

    ui->fsResults->setContextMenuPolicy(Qt::CustomContextMenu);

    m_zoomGroup = new QButtonGroup(this);
    m_zoomGroup->addButton(ui->btnZoomFit, ZMangaView::zmFit);
    m_zoomGroup->addButton(ui->btnZoomWidth, ZMangaView::zmWidth);
    m_zoomGroup->addButton(ui->btnZoomHeight, ZMangaView::zmHeight);
    m_zoomGroup->addButton(ui->btnZoomOriginal, ZMangaView::zmOriginal);
    m_zoomGroup->setExclusive(true);

    connect(ui->actionExit,&QAction::triggered,this,&ZMainWindow::close);
    connect(ui->actionOpen,&QAction::triggered,this,&ZMainWindow::openAux);
    connect(ui->actionOpenClipboard,&QAction::triggered,this,&ZMainWindow::openClipboard);
    connect(ui->actionClose,&QAction::triggered,this,&ZMainWindow::closeManga);
    connect(ui->actionSettings,&QAction::triggered,zF->global(),&ZGlobal::settingsDlg);
    connect(ui->actionAddBookmark,&QAction::triggered,this,&ZMainWindow::createBookmark);
    connect(ui->actionAbout,&QAction::triggered,this,&ZMainWindow::helpAbout);
    connect(ui->actionFullscreen,&QAction::triggered,this,&ZMainWindow::switchFullscreen);
    connect(ui->actionMinimize,&QAction::triggered,this,&ZMainWindow::showMinimized);
    connect(ui->actionSaveSettings,&QAction::triggered,zF->global(),&ZGlobal::saveSettings);
    connect(ui->actionShowOCR,&QAction::triggered,zF->global()->ocrEditor(),&ZOCREditor::show);

    connect(ui->searchTab,&ZSearchTab::mangaDblClick,this,&ZMainWindow::openFromIndex);
    connect(ui->searchTab,&ZSearchTab::statusBarMsg,this,&ZMainWindow::auxMessage);
    connect(ui->tabWidget,&QTabWidget::currentChanged,this,&ZMainWindow::tabChanged);
    connect(ui->fastScrollSlider,&QSlider::valueChanged,this,&ZMainWindow::fastScroll);

    connect(ui->btnOpen,&QPushButton::clicked,this,&ZMainWindow::openAux);
    connect(ui->btnRotateCCW,&QPushButton::clicked,ui->mangaView,&ZMangaView::viewRotateCCW);
    connect(ui->btnRotateCW,&QPushButton::clicked,ui->mangaView,&ZMangaView::viewRotateCW);
    connect(ui->btnNavFirst,&QPushButton::clicked,ui->mangaView,&ZMangaView::navFirst);
    connect(ui->btnNavPrev,&QPushButton::clicked,ui->mangaView,&ZMangaView::navPrev);
    connect(ui->btnNavNext,&QPushButton::clicked,ui->mangaView,&ZMangaView::navNext);
    connect(ui->btnNavLast,&QPushButton::clicked,ui->mangaView,&ZMangaView::navLast);
    connect(ui->btnZoomDynamic,&QPushButton::toggled,ui->mangaView,&ZMangaView::setZoomDynamic);
    connect(ui->btnSearchTab,&QPushButton::clicked,this,&ZMainWindow::openSearchTab);
    connect(ui->btnFSAdd,&QPushButton::clicked,this,&ZMainWindow::fsAddFiles);
    connect(ui->btnFSCheck,&QPushButton::clicked,this,&ZMainWindow::fsCheckAvailability);
    connect(ui->btnFSDelete,&QPushButton::clicked,this,&ZMainWindow::fsDelFiles);
    connect(ui->btnFSFind,&QPushButton::clicked,this,&ZMainWindow::fsFindNewFiles);
    connect(m_zoomGroup,&QButtonGroup::idClicked,this,[this](int mode){
        ui->comboZoom->setEnabled(mode==ZMangaView::zmOriginal);
        ui->mangaView->setZoomMode(mode);
    });

    connect(ui->scrollArea,&ZScrollArea::sizeChanged,ui->mangaView,&ZMangaView::ownerResized);
    connect(ui->comboZoom,&QComboBox::currentIndexChanged,ui->mangaView,&ZMangaView::setZoomAny);
    connect(ui->spinPosition,&QSpinBox::editingFinished,this,&ZMainWindow::pageNumEdited);
    connect(ui->comboMouseMode,qOverload<int>(&QComboBox::currentIndexChanged),
            this,&ZMainWindow::changeMouseMode);

    connect(ui->mangaView,&ZMangaView::loadedPage,this,&ZMainWindow::dispPage);
    connect(ui->mangaView,&ZMangaView::doubleClicked,this,&ZMainWindow::switchFullscreen);
    connect(ui->mangaView,&ZMangaView::keyPressed,this,&ZMainWindow::viewerKeyPressed);
    connect(ui->mangaView,&ZMangaView::minimizeRequested,this,&ZMainWindow::showMinimized);
    connect(ui->mangaView,&ZMangaView::auxMessage,this,&ZMainWindow::auxMessage);
    connect(ui->mangaView,&ZMangaView::averageSizes,this,&ZMainWindow::msgFromMangaView);
    connect(ui->mangaView,&ZMangaView::rotationUpdated,this,&ZMainWindow::rotationUpdated);
    connect(ui->mangaView,&ZMangaView::cropUpdated,this,&ZMainWindow::cropUpdated);
    connect(ui->mangaView,&ZMangaView::backgroundUpdated,this,&ZMainWindow::viewerBackgroundUpdated);

    connect(ui->fsResults,&QTableWidget::customContextMenuRequested,
            this,&ZMainWindow::fsResultsMenuCtx);

    connect(zF->global(),&ZGlobal::auxMessage,this,&ZMainWindow::auxMessage);
    // Explicitly use DirectConnection for synchronous call
    connect(zF->global(),&ZGlobal::loadSearchTabSettings,ui->searchTab,&ZSearchTab::loadSettings,Qt::DirectConnection);
    connect(zF->global(),&ZGlobal::saveSearchTabSettings,ui->searchTab,&ZSearchTab::saveSettings,Qt::DirectConnection);

    connect(zF->global()->db(),&ZDB::foundNewFiles,this,&ZMainWindow::fsFoundNewFiles,Qt::QueuedConnection);
    connect(zF->global(),&ZGlobal::fsFilesAdded,this,&ZMainWindow::fsNewFilesAdded);

    connect(this,&ZMainWindow::dbAddIgnoredFiles,zF->global()->db(),&ZDB::sqlAddIgnoredFiles,Qt::QueuedConnection);
    connect(this,&ZMainWindow::dbFindNewFiles,zF->global()->db(),&ZDB::sqlSearchMissingManga,Qt::QueuedConnection);
    connect(this,&ZMainWindow::dbAddFiles,zF->global()->db(),&ZDB::sqlAddFiles,Qt::QueuedConnection);

    ui->mangaView->setScroller(ui->scrollArea);
    zF->global()->loadSettings();
    centerWindow(!isMaximized());
    m_savedGeometry=geometry();
    m_savedMaximized=isMaximized();
    dispPage(-1,QString());

    QString fname;
    if (QApplication::arguments().count()>1)
        fname = QApplication::arguments().at(1);
    if (!fname.isEmpty() && !fname.startsWith(QSL("--")))
        openAuxFile(fname);

    ui->btnZoomFit->click();

#ifdef WITH_OCR
    if (!zF->isOCRReady()) {
        QMessageBox::critical(nullptr,QGuiApplication::applicationDisplayName(),
                              tr("Could not initialize Tesseract. \n"
                                 "Maybe language training data is not installed."));
    }
#endif
}

ZMainWindow::~ZMainWindow()
{
    delete ui;
}

void ZMainWindow::centerWindow(bool moveWindow)
{
    constexpr int initialHeightFrac = 80;
    constexpr int initialWidthFrac = 135;
    constexpr int maxWidth = 1000;
    constexpr int maxWidthFrac = 80;

    QScreen *screen = nullptr;

    if (window() && window()->windowHandle()) {
        screen = window()->windowHandle()->screen();
    } else if (!QApplication::screens().isEmpty()) {
        screen = QApplication::screenAt(QCursor::pos());
    }
    if (screen == nullptr)
        screen = QApplication::primaryScreen();

    QRect rect(screen->availableGeometry());
    int h = initialHeightFrac*rect.height()/100;
    QSize nw(initialWidthFrac*h/100,h);
    if (nw.width()<maxWidth) nw.setWidth(maxWidthFrac*rect.width()/100);

    if (moveWindow) {
        resize(nw);
        move(rect.width()/2 - frameGeometry().width()/2,
             rect.height()/2 - frameGeometry().height()/2);
        ui->searchTab->updateSplitters();
    }

    zF->global()->setDPI(screen->physicalDotsPerInchX(),screen->physicalDotsPerInchY());
#ifdef Q_OS_WIN
    const qreal maxDPIdifference = 20.0;
    const qreal minDPI = 130.0;
    if (abs(zF->global()->getDpiX() - zF->global()->getDpiY())>maxDPIdifference) {
        qreal dpi = qMax(zF->global()->getDpiX(),zF->global()->getDpiY());
        if (dpi<minDPI) dpi=minDPI;
        zF->global()->setDPI(dpi,dpi);
    }
#endif
}

bool ZMainWindow::isMangaOpened()
{
    return (ui->mangaView->getPageCount()>0);
}

bool ZMainWindow::isFullScreenControlsVisible() const
{
    return !m_fullScreen || m_fullScreenControls;
}

void ZMainWindow::setZoomMode(int mode)
{
    auto *btn = m_zoomGroup->button(mode);
    if (btn)
        btn->click();
}

void ZMainWindow::addContextMenuItems(QMenu* menu)
{
    auto *nt = new QAction(QIcon(QSL(":/16/zoom-fit-best")),tr("Zoom fit"),nullptr);
    connect(nt,&QAction::triggered,this,[this](){
        setZoomMode(ZMangaView::zmFit);
    });
    menu->addAction(nt);
    nt = new QAction(QIcon(QSL(":/16/zoom-fit-width")),tr("Zoom width"),nullptr);
    connect(nt,&QAction::triggered,this,[this](){
        setZoomMode(ZMangaView::zmWidth);
    });
    menu->addAction(nt);
    nt = new QAction(QIcon(QSL(":/16/zoom-fit-height")),tr("Zoom height"),nullptr);
    connect(nt,&QAction::triggered,this,[this](){
        setZoomMode(ZMangaView::zmHeight);
    });
    menu->addAction(nt);
    nt = new QAction(QIcon(QSL(":/16/zoom-original")),tr("Zoom original"),nullptr);
    connect(nt,&QAction::triggered,this,[this](){
        setZoomMode(ZMangaView::zmOriginal);
    });
    menu->addAction(nt);
    menu->addSeparator();
    nt = new QAction(QIcon(QSL(":/16/zoom-draw")),tr("Zoom dynamic"),nullptr);
    nt->setCheckable(true);
    nt->setChecked(ui->mangaView->getZoomDynamic());
    connect(nt,&QAction::triggered,this,[this](){
        ui->btnZoomDynamic->click();
    });
    menu->addAction(nt);
    menu->addSeparator();

    nt = new QAction(QIcon(QSL(":/16/document-close")),tr("Close manga"),nullptr);
    connect(nt,&QAction::triggered,this,&ZMainWindow::closeManga);
    menu->addAction(nt);
    menu->addSeparator();
    nt = new QAction(QIcon(QSL(":/16/go-down")),tr("Minimize window"),nullptr);
    connect(nt,&QAction::triggered,this,&ZMainWindow::showMinimized);
    menu->addAction(nt);
    nt = new QAction(QIcon(QSL(":/16/transform-move")),tr("Show fast scroller"),nullptr);
    nt->setCheckable(true);
    nt->setChecked(ui->fastScrollPanel->isVisible());
    connect(nt,&QAction::toggled,ui->fastScrollPanel,&QFrame::setVisible);
    menu->addAction(nt);
    nt = new QAction(QIcon(QSL(":/16/edit-rename")),tr("Show controls"),nullptr);
    nt->setCheckable(true);
    nt->setShortcut(QKeySequence(Qt::Key_Return));
    nt->setChecked(isFullScreenControlsVisible());
    connect(nt,&QAction::toggled,this,&ZMainWindow::switchFullscreenControls);
    menu->addAction(nt);
}

void ZMainWindow::openAuxFile(const QString &filename)
{
    QFileInfo fi(filename);
    zF->global()->setSavedAuxOpenDir(fi.path());

    ui->tabWidget->setCurrentIndex(0);
    ui->mangaView->openFile(filename);
}

void ZMainWindow::closeEvent(QCloseEvent *event)
{
    if (zF->global()!=nullptr)
        zF->global()->saveSettings();
    event->accept();
}

void ZMainWindow::openAux()
{
    QString filename = zF->getOpenFileNameD(this,tr("Open manga or image"),zF->global()->getSavedAuxOpenDir());
    if (!filename.isEmpty())
        openAuxFile(filename);
}

void ZMainWindow::openClipboard()
{
    if (!QApplication::clipboard()->mimeData()->hasImage()) {
        QMessageBox::warning(this,QGuiApplication::applicationDisplayName(),
                             tr("Clipboard is empty or contains incompatible data."));
        return;
    }

    QImage img = QApplication::clipboard()->image();
    QByteArray imgs;
    QBuffer buf(&imgs);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf,"BMP");
    buf.close();
    QString bs = QSL(":CLIP:")+QString::fromLatin1(imgs.toBase64());

    ui->tabWidget->setCurrentIndex(0);
    ui->mangaView->openFile(bs);
}

void ZMainWindow::openFromIndex(const QString &filename)
{
    ui->tabWidget->setCurrentIndex(0);
    ui->mangaView->openFile(filename);
}

void ZMainWindow::closeManga()
{
    ui->mangaView->closeFile();
    ui->spinPosition->hide();
}

void ZMainWindow::dispPage(int num, const QString &msg)
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

    if (!msg.isEmpty()) {
        statusBar()->showMessage(msg);
    } else {
        statusBar()->clearMessage();
    }
}

void ZMainWindow::switchFullscreen()
{
    m_fullScreen = !m_fullScreen;
    m_fullScreenControls = false;

    if (m_fullScreen) {
        m_savedMaximized = isMaximized();
        m_savedGeometry = geometry();
    }

    updateControlsVisibility();
    int m = 2;
    if (m_fullScreen) m = 0;
    if (ui->centralWidget != nullptr)
        ui->centralWidget->layout()->setContentsMargins(m,m,m,m);
    if (ui->tabWidget->currentWidget()!=nullptr)
        ui->tabWidget->currentWidget()->layout()->setContentsMargins(m,m,m,m);
    if (m_fullScreen) {
        showFullScreen();
    } else {
        if (m_savedMaximized) {
            showNormal();
            showMaximized();
        } else {
            showNormal();
            setGeometry(m_savedGeometry);
        }
    }
    ui->actionFullscreen->setChecked(m_fullScreen);
}

void ZMainWindow::switchFullscreenControls()
{
    if (!m_fullScreen) return;

    m_fullScreenControls = !m_fullScreenControls;
    updateControlsVisibility();
}

void ZMainWindow::updateControlsVisibility()
{
    bool state = isFullScreenControlsVisible();

    statusBar()->setVisible(state);
    menuBar()->setVisible(state);
    ui->tabWidget->tabBar()->setVisible(state);
    ui->toolbar->setVisible(state);
}

void ZMainWindow::viewerKeyPressed(int key)
{
    if (m_fullScreen) {
        if (key==Qt::Key_Return) {
            switchFullscreenControls();
        }
        if (QKeySequence(key)==ui->actionFullscreen->shortcut()) {
            switchFullscreen();
        }
    }
}

void ZMainWindow::updateViewer()
{
    ui->mangaView->redrawPage();
}

void ZMainWindow::rotationUpdated(double angle)
{
    m_lblRotation->setText(tr("Rotation: %1").arg(qRadiansToDegrees(angle),1,'f',1));
}

void ZMainWindow::cropUpdated(const QRect &crop)
{
    if (!crop.isNull()) {
        m_lblCrop->setText(tr("Cropped: %1x%2").arg(crop.width()).arg(crop.height()));
    } else {
        m_lblCrop->clear();
    }
}

void ZMainWindow::fastScroll(int page)
{
    if (!isMangaOpened()) return;
    if (page<=0 || page>ui->mangaView->getPageCount()) return;
    ui->mangaView->setPage(page-1);
}

void ZMainWindow::updateFastScrollPosition()
{
    if (!isMangaOpened()) return;
    ui->fastScrollSlider->blockSignals(true);
    ui->fastScrollSlider->setValue(ui->mangaView->getCurrentPage()+1);
    ui->fastScrollSlider->blockSignals(false);
}

void ZMainWindow::tabChanged(int idx)
{
    if (idx==1 && ui->searchTab!=nullptr)
        ui->searchTab->updateFocus();
}

void ZMainWindow::changeMouseMode(int mode)
{
    switch (mode) {
        case 0: ui->mangaView->setMouseMode(ZMangaView::mmOCR); break;
        case 1: ui->mangaView->setMouseMode(ZMangaView::mmPan); break;
        case 2: ui->mangaView->setMouseMode(ZMangaView::mmCrop); break;
        default:
            qWarning() << "Incorrect mouse mode selected";
            break;
    }
}

void ZMainWindow::viewerBackgroundUpdated(const QColor &color)
{
    Q_UNUSED(color)
    static QColor oldColor(Qt::black);

    QColor c = palette().window().color();
    if (c != oldColor) {
        QPalette p = ui->fastScrollSlider->palette();
        p.setBrush(QPalette::Window, QBrush(zF->global()->getBackgroundColor()));
        ui->fastScrollSlider->setPalette(p);
    }

    oldColor = c;
}

void ZMainWindow::updateBookmarks()
{
    while (ui->menuBookmarks->actions().count()>2)
        ui->menuBookmarks->removeAction(ui->menuBookmarks->actions().constLast());
    const auto bookmarks = zF->global()->getBookmarks();
    for (auto it = bookmarks.constKeyValueBegin(), end = bookmarks.constKeyValueEnd();
         it != end; ++it) {
        QAction* a = ui->menuBookmarks->addAction((*it).first,this,&ZMainWindow::openBookmark);
        QString st = (*it).second;
        a->setData(st);
        if (st.split('\n').count()>0)
            st = st.split('\n').at(0);
        a->setStatusTip(st);
    }
}

void ZMainWindow::updateTitle()
{
    QString t;
    t.clear();
    const QString openedFile = ui->mangaView->getOpenedFile();
    if (!openedFile.isEmpty()) {
        QFileInfo fi(openedFile);
        t = tr("%1 - QManga").arg(fi.fileName());
    } else {
        t = tr("QManga - Manga viewer and indexer");
    }
    setWindowTitle(t);
}

void ZMainWindow::createBookmark()
{
    const QString openedFile = ui->mangaView->getOpenedFile();
    if (openedFile.isEmpty()) return;
    QFileInfo fi(openedFile);
    ZTwoEditDlg dlg(this,tr("Add bookmark"),tr("Title"),tr("Filename"),
                                       fi.completeBaseName(),openedFile);
    if (dlg.exec()==QDialog::Accepted) {
        QString t = dlg.getDlgEdit1();
        if (!t.isEmpty() && !zF->global()->getBookmarks().contains(t)) {
            zF->global()->addBookmark(t,QSL("%1\n%2").arg(dlg.getDlgEdit2()).arg(ui->mangaView->getCurrentPage()));
            updateBookmarks();
        } else {
            QMessageBox::warning(this,QGuiApplication::applicationDisplayName(),
                                 tr("Unable to add bookmark (frame is empty or duplicate title). Try again."));
        }
    }
}

void ZMainWindow::openBookmark()
{
    auto *a = qobject_cast<QAction *>(sender());
    if (a==nullptr) return;

    QString f = a->data().toString();
    QStringList sl = a->data().toString().split('\n');
    int page = 0;
    if (sl.count()>1) {
        f = sl.at(0);
        bool okconv = false;
        page = sl.at(1).toInt(&okconv);
        if (!okconv) page = 0;
    }
    QFileInfo fi(f);
    if (!fi.isReadable()) {
        QMessageBox::warning(this,QGuiApplication::applicationDisplayName(),
                             tr("Unable to open file %1").arg(f));
        return;
    }

    ui->tabWidget->setCurrentIndex(0);
    ui->mangaView->openFileEx(f,page);
}

void ZMainWindow::openSearchTab()
{
    ui->tabWidget->setCurrentIndex(1);
}

void ZMainWindow::helpAbout()
{
    QMessageBox::about(this, QGuiApplication::applicationDisplayName(),
                       tr("Manga reader with MySQL indexer.\n\nLicensed under GPL v3 license.\n\n"
                          "Author: kernelonline.\n"
                          "App icon (Alien9) designer: EXO.\n"
                          "Icon pack: KDE team Oxygen project."));

}

void ZMainWindow::auxMessage(const QString &msg)
{
    QString s = msg;
    bool showMsgBox = s.startsWith(QSL("MBOX#"));
    if (showMsgBox)
        s.remove(QSL("MBOX#"));
    m_lblSearchStatus->setText(s);
    if (showMsgBox) {
        if (m_indexerMsgBox.isVisible()) {
            m_indexerMsgBox.setText(m_indexerMsgBox.text()+QSL("\n")+s);
        } else {
            m_indexerMsgBox.setText(s);
        }
        m_indexerMsgBox.exec();
    }
}

void ZMainWindow::msgFromMangaView(const QSize &sz, qint64 fsz)
{
    if (zF->global()->getCachePixmaps()) {
        m_lblAverageSizes->setText(tr("Avg size: %1x%2").arg(sz.width()).arg(sz.height()));
    } else {
        m_lblAverageSizes->setText(tr("Avg size: %1x%2, %3").arg(sz.width()).arg(sz.height())
                                   .arg(zF->formatSize(fsz)));
    }

    if (zF->global()->getAvgFineRenderTime()>0)
        m_lblAverageFineRender->setText(tr("Avg filter: %1 ms").arg(zF->global()->getAvgFineRenderTime()));
}

void ZMainWindow::fsAddFiles()
{
    QHash<QString,QStringList> fl;
    fl.clear();
    for (const auto &i : qAsConst(m_fsScannedFiles)) {
        fl[i.album].append(i.fileName);
        zF->global()->removeFileFromNewlyAdded(i.fileName);
    }

    for (auto it = fl.constKeyValueBegin(), end = fl.constKeyValueEnd(); it != end; ++it)
        Q_EMIT dbAddFiles((*it).second,(*it).first);

    zF->global()->fsCheckFilesAvailability();
}

void ZMainWindow::fsCheckAvailability()
{
    zF->global()->fsCheckFilesAvailability();
}

void ZMainWindow::fsDelFiles()
{
    QVector<int> rows;
    const QList<QTableWidgetItem *> list = ui->fsResults->selectedItems();
    rows.reserve(list.count());
    for (const auto &i : list) {
        int idx = i->row();
        if (!rows.contains(idx))
            rows << idx;
    }
    if (rows.isEmpty()) return;
    QVector<ZFSFile> nl;
    for (int i=0;i<m_fsScannedFiles.count();i++) {
        if (!rows.contains(i))
            nl << m_fsScannedFiles.at(i);
    }
    m_fsScannedFiles = nl;
    fsUpdateFileList();
}

void ZMainWindow::fsNewFilesAdded()
{
    m_fsAddFilesMutex.lock();
    ui->fsResults->clear();
    m_fsScannedFiles.clear();
    const QStringList f = zF->global()->getNewlyAddedFiles();

    for (const auto &i : f) {
        QFileInfo fi(i);
        bool mimeOk = false;
        zF->readerFactory(this,fi.absoluteFilePath(),&mimeOk,true,false);
        if (mimeOk)
            m_fsScannedFiles << ZFSFile(fi.fileName(),fi.absoluteFilePath(),fi.absoluteDir().dirName());
    }
    fsUpdateFileList();
    m_fsAddFilesMutex.unlock();
}

void ZMainWindow::fsUpdateFileList()
{
    QStringList h;
    h << tr("File") << tr("Album");
    ui->fsResults->setColumnCount(2);
    ui->fsResults->setRowCount(m_fsScannedFiles.count());
    ui->fsResults->setHorizontalHeaderLabels(h);
    for (int i=0;i<m_fsScannedFiles.count();i++) {
        ui->fsResults->setItem(i,0,new QTableWidgetItem(m_fsScannedFiles[i].name));
        ui->fsResults->setItem(i,1,new QTableWidgetItem(m_fsScannedFiles[i].album));
        QTableWidgetItem* w = ui->fsResults->item(i,0);
        if (w!=nullptr)
                w->setToolTip(m_fsScannedFiles[i].fileName);
    }
    ui->fsResults->setColumnWidth(0,ui->tabWidget->width()/2);
}

void ZMainWindow::fsResultsMenuCtx(const QPoint &pos)
{
    QMenu cm(this);
    cm.setStyleSheet(QSL("QMenu { menu-scrollable: 1; }"));
    QAction* ac = nullptr;
    int cnt=0;
    if (!ui->fsResults->selectedItems().isEmpty()) {
        ac = new QAction(QIcon(QSL(":/16/dialog-cancel")),tr("Ignore these file(s)"),this);
        connect(ac,&QAction::triggered,this,&ZMainWindow::fsAddIgnoredFiles);
        cm.addAction(ac);
        cm.addSeparator();
        cnt++;
    }
    const QStringList albums = ui->searchTab->getAlbums();
    for (const auto &i : albums) {
        if (i.startsWith(QSL("#"))) continue;
        ac = new QAction(i,nullptr);
        connect(ac,&QAction::triggered,this,&ZMainWindow::fsResultsCtxApplyAlbum);
        ac->setData(i);
        cm.addAction(ac);
        cnt++;
    }
    if (cnt>0)
        cm.exec(ui->fsResults->mapToGlobal(pos));
}

void ZMainWindow::fsResultsCtxApplyAlbum()
{
    auto *ac = qobject_cast<QAction *>(sender());
    if (ac==nullptr) return;
    QString s = ac->data().toString();
    if (s.isEmpty()) {
        QMessageBox::warning(this,QGuiApplication::applicationDisplayName(),
                             tr("Unable to apply album name"));
        return;
    }
    QVector<int> idxs;
    const QList<QTableWidgetItem *> list = ui->fsResults->selectedItems();
    idxs.reserve(list.count());
    for (const auto &i : list)
        idxs << i->row();

    for (const auto &i : qAsConst(idxs))
        m_fsScannedFiles[i].album = s;

    fsUpdateFileList();
}

void ZMainWindow::fsFindNewFiles()
{
    m_fsScannedFiles.clear();
    fsUpdateFileList();
    ui->searchTab->dbShowProgressDialogEx(true,tr("Scanning filesystem"));
    ui->searchTab->dbShowProgressState(ZDefaults::fileScannerStaticPercentage,
                                       tr("Scanning filesystem..."));
    Q_EMIT dbFindNewFiles();
}

void ZMainWindow::fsFoundNewFiles(const QStringList &files)
{
    for (const QString& filename : files) {
        QFileInfo fi(filename);
        bool mimeOk = false;
        zF->readerFactory(this,fi.absoluteFilePath(),&mimeOk,true,false);
        if (mimeOk)
            m_fsScannedFiles << ZFSFile(fi.fileName(),fi.absoluteFilePath(),fi.absoluteDir().dirName());
    }
    fsUpdateFileList();
    ui->searchTab->dbShowProgressDialog(false);
}

void ZMainWindow::fsAddIgnoredFiles()
{
    QList<int> rows;
    const QList<QTableWidgetItem *> list = ui->fsResults->selectedItems();
    rows.reserve(list.count());
    for (const auto &i : list) {
        int idx = i->row();
        if (!rows.contains(idx))
            rows << idx;
    }
    if (rows.isEmpty()) return;
    QVector<ZFSFile> nl;
    QStringList sl;
    for (int i=0;i<m_fsScannedFiles.count();i++) {
        if (!rows.contains(i)) {
            nl << m_fsScannedFiles.at(i);
        } else {
            sl << m_fsScannedFiles.at(i).fileName;
        }
    }
    Q_EMIT dbAddIgnoredFiles(sl);
    m_fsScannedFiles = nl;
    fsUpdateFileList();
}

void ZMainWindow::pageNumEdited()
{
    if (ui->spinPosition->value()!=ui->mangaView->getCurrentPage())
        ui->mangaView->setPage(ui->spinPosition->value()-1);
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
