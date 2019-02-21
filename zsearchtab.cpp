#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDirIterator>
#include <QCompleter>
#include <QProgressDialog>
#include <QApplication>
#include <QDesktopServices>
#include <QClipboard>
#include <QMimeData>
#include <QScrollBar>
#include <QDebug>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "zfilecopier.h"
#include "zglobal.h"
#include "zmangamodel.h"
#include "albumselectordlg.h"
#include "zsearchtab.h"
#include "ui_zsearchtab.h"

#define STACK_ICONS 0
#define STACK_TABLE 1

ZSearchTab::ZSearchTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ZSearchTab)
{
    ui->setupUi(this);

    cachedAlbums.clear();

    progressDlg = nullptr;

    descTemplate = ui->srcDesc->toHtml();
    ui->srcDesc->clear();
    ui->srcLoading->hide();

    auto completer = new QCompleter(this);
    searchHistoryModel = new ZMangaSearchHistoryModel(completer);
    completer->setModel(searchHistoryModel);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    ui->srcEdit->setCompleter(completer);

    ui->srcIconSize->setMinimum(16);
    ui->srcIconSize->setMaximum(maxPreviewSize);
    ui->srcIconSize->setValue(128);
    ui->srcList->setGridSize(gridSize(ui->srcIconSize->value()));
    ui->srcList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->srcModeIcon->setChecked(ui->srcStack->currentIndex()==STACK_ICONS);
    ui->srcModeList->setChecked(ui->srcStack->currentIndex()==STACK_TABLE);
    ui->srcAlbums->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->srcAlbums,&QListWidget::itemClicked,this,&ZSearchTab::albumClicked);
    connect(ui->srcAlbums,&QListWidget::customContextMenuRequested,
            this,&ZSearchTab::ctxAlbumMenu);
    connect(ui->srcList,&QListView::doubleClicked,this,&ZSearchTab::mangaOpen);
    connect(ui->srcList,&QListView::customContextMenuRequested,this,&ZSearchTab::ctxMenu);
    connect(ui->srcTable,&QTableView::doubleClicked,this,&ZSearchTab::mangaOpen);
    connect(ui->srcTable,&QTableView::customContextMenuRequested,this,&ZSearchTab::ctxMenu);
    connect(ui->srcAddBtn,&QPushButton::clicked,this,&ZSearchTab::mangaAdd);
    connect(ui->srcAddDirBtn,&QPushButton::clicked,this,&ZSearchTab::mangaAddDir);
    connect(ui->srcAddImgDirBtn,&QPushButton::clicked,this,&ZSearchTab::imagesAddDir);
    connect(ui->srcDelBtn,&QPushButton::clicked,this,&ZSearchTab::mangaDel);
    connect(ui->srcModeIcon,&QRadioButton::toggled,this,&ZSearchTab::listModeChanged);
    connect(ui->srcModeList,&QRadioButton::toggled,this,&ZSearchTab::listModeChanged);
    connect(ui->srcIconSize,&QSlider::valueChanged,this,&ZSearchTab::iconSizeChanged);
    connect(ui->srcEdit,&QLineEdit::returnPressed,ui->srcEditBtn,&QPushButton::click);
    connect(ui->srcEditBtn,&QPushButton::clicked,this,&ZSearchTab::mangaSearch);

    connect(zg->db,&ZDB::albumsListUpdated,this,&ZSearchTab::dbAlbumsListUpdated,Qt::QueuedConnection);
    connect(zg->db,&ZDB::gotAlbums,this,&ZSearchTab::dbAlbumsListReady,Qt::QueuedConnection);
    connect(this,&ZSearchTab::dbRenameAlbum,zg->db,&ZDB::sqlRenameAlbum,Qt::QueuedConnection);
    connect(zg->db,&ZDB::filesAdded,this,&ZSearchTab::dbFilesAdded,Qt::QueuedConnection);
    connect(zg->db,&ZDB::filesLoaded,this,&ZSearchTab::dbFilesLoaded,Qt::QueuedConnection);
    connect(zg->db,&ZDB::errorMsg,this,&ZSearchTab::dbErrorMsg,Qt::QueuedConnection);
    connect(zg->db,&ZDB::needTableCreation,this,&ZSearchTab::dbNeedTableCreation,Qt::QueuedConnection);
    connect(zg->db,&ZDB::showProgressDialog,this,&ZSearchTab::dbShowProgressDialog,Qt::QueuedConnection);
    connect(zg->db,&ZDB::showProgressState,this,&ZSearchTab::dbShowProgressState,Qt::QueuedConnection);

    connect(this,&ZSearchTab::dbAddFiles,zg->db,&ZDB::sqlAddFiles,Qt::QueuedConnection);
    connect(this,&ZSearchTab::dbDelFiles,zg->db,&ZDB::sqlDelFiles,Qt::QueuedConnection);
    connect(this,&ZSearchTab::dbGetAlbums,zg->db,&ZDB::sqlGetAlbums,Qt::QueuedConnection);
    connect(this,&ZSearchTab::dbGetFiles,zg->db,&ZDB::sqlGetFiles,Qt::QueuedConnection);
    connect(this,&ZSearchTab::dbRenameAlbum,zg->db,&ZDB::sqlRenameAlbum,Qt::QueuedConnection);
    connect(this,&ZSearchTab::dbCreateTables,zg->db,&ZDB::sqlCreateTables,Qt::QueuedConnection);
    connect(this,&ZSearchTab::dbDeleteAlbum,zg->db,&ZDB::sqlDelAlbum,Qt::QueuedConnection);

    auto sLoaded = new QState();
    auto sLoading = new QState();
    sLoading->addTransition(zg->db,&ZDB::filesLoaded,sLoaded);
    sLoaded->addTransition(this,&ZSearchTab::dbGetFiles,sLoading);
    sLoading->assignProperty(ui->srcAddBtn,"enabled",false);
    sLoading->assignProperty(ui->srcAddDirBtn,"enabled",false);
    sLoading->assignProperty(ui->srcDelBtn,"enabled",false);
    sLoading->assignProperty(ui->srcEditBtn,"enabled",false);
    sLoading->assignProperty(ui->srcEdit,"enabled",false);
    sLoading->assignProperty(ui->srcAlbums,"enabled",false);
    sLoading->assignProperty(ui->srcLoading,"visible",true);
    sLoaded->assignProperty(ui->srcAddBtn,"enabled",true);
    sLoaded->assignProperty(ui->srcAddDirBtn,"enabled",true);
    sLoaded->assignProperty(ui->srcDelBtn,"enabled",true);
    sLoaded->assignProperty(ui->srcEditBtn,"enabled",true);
    sLoaded->assignProperty(ui->srcEdit,"enabled",true);
    sLoaded->assignProperty(ui->srcAlbums,"enabled",true);
    sLoaded->assignProperty(ui->srcLoading,"visible",false);
    loadingState.addState(sLoading);
    loadingState.addState(sLoaded);
    loadingState.setInitialState(sLoaded);
    loadingState.start();

    auto aModel = new ZMangaModel(this,ui->srcIconSize,ui->srcTable);
    tableModel = new ZMangaTableModel(this,ui->srcTable);
    iconModel = new ZMangaIconModel(this,ui->srcList);
    tableModel->setSourceModel(aModel);
    iconModel->setSourceModel(aModel);
    ui->srcList->setModel(iconModel);
    ui->srcTable->setModel(tableModel);
    connect(ui->srcTable->horizontalHeader(),&QHeaderView::sortIndicatorChanged,this,
            [this](int logicalIndex, Qt::SortOrder order){
        iconModel->sort(logicalIndex,order);
    });

    connect(ui->srcList->selectionModel(),&QItemSelectionModel::currentChanged,
            this,&ZSearchTab::mangaSelectionChanged);
    connect(ui->srcTable->selectionModel(),&QItemSelectionModel::currentChanged,
            this,&ZSearchTab::mangaSelectionChanged);

    connect(zg->db,&ZDB::gotFile,aModel,&ZMangaModel::addItem,Qt::QueuedConnection);
    connect(zg->db,&ZDB::deleteItemsFromModel,aModel,&ZMangaModel::deleteItems,Qt::QueuedConnection);
    model = aModel;

    ui->srcList->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->srcTable->setContextMenuPolicy(Qt::CustomContextMenu);

    updateSplitters();
}

ZSearchTab::~ZSearchTab()
{
    delete ui;
}

void ZSearchTab::updateSplitters()
{
    QList<int> widths;
    ui->splitter->setCollapsible(0,true);
    widths << 65;
    widths << width()-widths[0];
    ui->splitter->setSizes(widths);
}

void ZSearchTab::ctxMenu(const QPoint &pos)
{
    QAbstractItemView *view = activeView();
    QMenu cm(view);

    QAction *acm = cm.addAction(QIcon(":/16/edit-select-all"),tr("Select All"));
    connect(acm,&QAction::triggered,view,&QAbstractItemView::selectAll);

    cm.addSeparator();

    const QModelIndexList li = getSelectedIndexes();
    int cnt = 0;
    for (const auto &i : li) {
        if (i.column()==0)
            cnt++;
    }
    QMenu* dmenu = cm.addMenu(QIcon(":/16/edit-delete"),
                              tr("Delete selected %1 files").arg(cnt));
    acm = dmenu->addAction(tr("Delete only from database"),this,&ZSearchTab::mangaDel);
    acm->setEnabled(cnt>0 && !ui->srcLoading->isVisible());
    acm->setData(1);
    acm = dmenu->addAction(tr("Delete from database and filesystem"),this,&ZSearchTab::mangaDel);
    acm->setEnabled(cnt>0 && !ui->srcLoading->isVisible());
    acm->setData(2);

    acm = cm.addAction(QIcon(":/16/document-open-folder"),
                       tr("Open containing directory"),this,&ZSearchTab::ctxOpenDir);
    acm->setEnabled(cnt>0 && !ui->srcLoading->isVisible());

    cm.addAction(QIcon(":/16/fork"),
                 tr("Open with default DE action"),this,&ZSearchTab::ctxXdgOpen);

    cm.addSeparator();

#ifndef Q_OS_WIN
    cm.addAction(QIcon(":/16/edit-copy"),
                 tr("Copy files"),this,&ZSearchTab::ctxFileCopyClipboard);
#endif

    cm.addAction(QIcon(":/16/edit-copy"),
                 tr("Copy files to directory..."),this,&ZSearchTab::ctxFileCopy);

    if (li.count()==1) {
        SQLMangaEntry m = model->getItem(li.first().row());
        if (m.fileMagic == "PDF") {
            cm.addSeparator();
            dmenu = cm.addMenu(tr("Preferred PDF rendering"));

            acm = dmenu->addAction(tr("Autodetect"));
            acm->setCheckable(true);
            acm->setChecked(m.rendering==Z::PDFRendering::Autodetect);
            acm->setData(static_cast<int>(Z::PDFRendering::Autodetect));
            connect(acm,&QAction::triggered,this,&ZSearchTab::ctxChangeRenderer);
            acm = dmenu->addAction(tr("Force rendering mode"));
            acm->setCheckable(true);
            acm->setChecked(m.rendering==Z::PDFRendering::PageRenderer);
            acm->setData(static_cast<int>(Z::PDFRendering::PageRenderer));
            connect(acm,&QAction::triggered,this,&ZSearchTab::ctxChangeRenderer);
            acm = dmenu->addAction(tr("Force catalog mode"));
            acm->setCheckable(true);
            acm->setChecked(m.rendering==Z::PDFRendering::ImageCatalog);
            acm->setData(static_cast<int>(Z::PDFRendering::ImageCatalog));
            connect(acm,&QAction::triggered,this,&ZSearchTab::ctxChangeRenderer);
        }
    }

    cm.exec(view->mapToGlobal(pos));
}

void ZSearchTab::ctxChangeRenderer()
{
    auto am = qobject_cast<QAction *>(sender());
    QModelIndexList li = getSelectedIndexes();
    if (am==nullptr || li.count()!=1) return;

    SQLMangaEntry m = model->getItem(li.first().row());

    bool ok;
    int mode = am->data().toInt(&ok);
    if (ok)
        QMetaObject::invokeMethod(zg->db,"sqlSetPreferredRendering",Qt::QueuedConnection,
                                  Q_ARG(QString,m.filename),Q_ARG(int,mode));
    else
        QMessageBox::warning(this,tr("QManga"),
                             tr("Unable to update preferred rendering."));

    albumClicked(ui->srcAlbums->currentItem());

}

void ZSearchTab::ctxAlbumMenu(const QPoint &pos)
{
    QListWidgetItem* itm = ui->srcAlbums->itemAt(pos);
    if (itm==nullptr) return;

    QMenu cm(ui->srcAlbums);
    QAction *acm = cm.addAction(QIcon(":/16/edit-rename"),tr("Rename album"),
                       this,&ZSearchTab::ctxRenameAlbum);
    acm->setData(itm->text());
    acm = cm.addAction(QIcon(":/16/edit-delete"),tr("Delete album"),
                       this,&ZSearchTab::ctxDeleteAlbum);
    acm->setData(itm->text());

    cm.exec(ui->srcAlbums->mapToGlobal(pos));
}

void ZSearchTab::ctxRenameAlbum()
{
    auto nt = qobject_cast<QAction *>(sender());
    if (nt==nullptr) return;
    QString s = nt->data().toString();
    if (s.isEmpty()) return;

    bool ok;
    QString n = QInputDialog::getText(this,tr("Rename album"),tr("New name for album '%1'").arg(s),
                                      QLineEdit::Normal,s,&ok);
    if (!ok) return;

    emit dbRenameAlbum(s,n);
}

void ZSearchTab::ctxDeleteAlbum()
{
    auto nt = qobject_cast<QAction *>(sender());
    if (nt==nullptr) return;
    QString s = nt->data().toString();
    if (s.isEmpty()) return;

    if (QMessageBox::question(this,tr("Delete album"),
                              tr("Are you sure to delete album '%1' and all it's contents from database?").arg(s),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No)==QMessageBox::Yes)
        emit dbDeleteAlbum(s);
}

void ZSearchTab::ctxOpenDir()
{
    const QFileInfoList fl = getSelectedMangaEntries(true);
    if (fl.isEmpty()) {
        QMessageBox::warning(this,tr("QManga"),
                             tr("Error while searching file path for some files."));
        return;
    }

    for (const auto &i : fl)
        QDesktopServices::openUrl(QUrl::fromLocalFile(i.path()));
}

void ZSearchTab::ctxXdgOpen()
{
    const QFileInfoList fl = getSelectedMangaEntries(true);
    if (fl.isEmpty()) {
        QMessageBox::warning(this,tr("QManga"),
                             tr("Error while searching file path for some files."));
        return;
    }

    for (const auto &i : fl)
        QDesktopServices::openUrl(QUrl::fromLocalFile(i.filePath()));
}

void ZSearchTab::ctxFileCopyClipboard()
{
    const QFileInfoList fl = getSelectedMangaEntries(true);
    if (fl.isEmpty()) return;

    QByteArray uriList;
    QByteArray plainList;

    for (const QFileInfo& fi : fl) {
        QByteArray ba = QUrl::fromLocalFile(fi.filePath()).toEncoded();
        if (!ba.isEmpty()) {
            uriList.append(ba); uriList.append('\x0D'); uriList.append('\x0A');
            plainList.append(ba); plainList.append('\x0A');
        }
    }

    if (uriList.isEmpty()) return;

    auto data = new QMimeData();
    data->setData("text/uri-list",uriList);
    data->setData("text/plain",plainList);
    data->setData("application/x-kde4-urilist",uriList);

    QClipboard *cl = qApp->clipboard();
    cl->clear();
    cl->setMimeData(data);
}

void ZSearchTab::ctxFileCopy()
{
    QFileInfoList fl = getSelectedMangaEntries(true);
    if (fl.isEmpty()) return;

    QString dst = getExistingDirectoryD(this,tr("Copy selected manga to..."),
                                                    zg->savedAuxSaveDir);
    if (dst.isEmpty()) return;
    zg->savedAuxSaveDir = dst;

    auto dlg = new QProgressDialog(this);
    dlg->setWindowModality(Qt::WindowModal);
    dlg->setWindowTitle(tr("Copying manga files..."));

    auto zfc = new ZFileCopier(fl,dlg,dst);
    auto zfc_thread = new QThread();

    connect(zfc,&ZFileCopier::errorMsg,this,&ZSearchTab::dbErrorMsg,Qt::QueuedConnection);
    connect(dlg,&QProgressDialog::canceled,zfc,&ZFileCopier::abort,Qt::QueuedConnection);

    connect(zfc_thread,&QThread::started,zfc,&ZFileCopier::start);
    connect(zfc,&ZFileCopier::finished,zfc_thread,&QThread::quit);
    connect(zfc,&ZFileCopier::finished,zfc,&QObject::deleteLater);
    connect(zfc_thread,&QThread::finished,zfc_thread,&QObject::deleteLater);

    zfc->moveToThread(zfc_thread);
    zfc_thread->start();
}

void ZSearchTab::updateAlbumsList()
{
    emit dbGetAlbums();
}

void ZSearchTab::updateFocus()
{
    if (activeView()->model()->rowCount()==0)
        ui->srcEdit->setFocus();
}

QStringList ZSearchTab::getAlbums()
{
    return cachedAlbums;
}

void ZSearchTab::setListViewOptions(const QListView::ViewMode mode, int iconSize)
{
    ui->srcModeList->setChecked(mode==QListView::ListMode);
    ui->srcModeIcon->setChecked(mode==QListView::IconMode);
    ui->srcIconSize->setValue(iconSize);
}

int ZSearchTab::getIconSize() const
{
    return ui->srcIconSize->value();
}

QListView::ViewMode ZSearchTab::getListViewMode() const
{
    if (ui->srcStack->currentIndex()==STACK_ICONS)
        return QListView::IconMode;

    return QListView::ListMode;
}

void ZSearchTab::loadSearchItems(QSettings &settings)
{
    if (searchHistoryModel==nullptr) return;

    QStringList sl;

    int sz=settings.beginReadArray("search_text");
    if (sz>0) {
        for (int i=0;i<sz;i++) {
            settings.setArrayIndex(i);
            QString s = settings.value("txt",QString()).toString();
            if (!s.isEmpty())
                sl << s;
            settings.endArray();
        }
    } else {
        settings.endArray();
        sl = settings.value("search_history",QStringList()).toStringList();
    }

    searchHistoryModel->setHistoryItems(sl);
}

void ZSearchTab::saveSearchItems(QSettings &settings)
{
    if (searchHistoryModel==nullptr) return;

    settings.setValue("search_history",QVariant::fromValue(searchHistoryModel->getHistoryItems()));
}

void ZSearchTab::applySortOrder(Z::Ordering order)
{
    int col = static_cast<int>(order);
    if (col>=0 && col<ui->srcTable->model()->columnCount())
        ui->srcTable->sortByColumn(col,Qt::AscendingOrder);
}

QSize ZSearchTab::gridSize(int ref)
{
    QFontMetrics fm(font());
    return QSize(ref+25,ref*previewProps+fm.height()*4);
}

QString ZSearchTab::getAlbumNameToAdd(const QString &suggest, int toAddCount)
{
    auto dlg = new AlbumSelectorDlg(this,cachedAlbums,suggest,toAddCount);
    QString ret = QString();
    if (dlg->exec()) {
        ret = dlg->listAlbums->lineEdit()->text();
    }
    dlg->setParent(nullptr);
    delete dlg;
    return ret;
}

QFileInfoList ZSearchTab::getSelectedMangaEntries(bool includeDirs) const
{
    QFileInfoList res;
    const QModelIndexList lii = getSelectedIndexes();
    if (lii.isEmpty()) return res;

    QModelIndexList li;
    for (const auto &i : lii) {
        if (i.column()==0)
            li << i;
    }

    for (const auto &i : qAsConst(li)) {
        if (i.row()>=0 && i.row()<model->getItemsCount()) {
            QString fname = model->getItem(i.row()).filename;
            QFileInfo fi(fname);
            if (fi.exists() && ((fi.isDir() && includeDirs) || fi.isFile())) {
                if (!res.contains(fi))
                    res << fi;
            }
        }
    }

    return res;
}

QAbstractItemView *ZSearchTab::activeView() const
{
    if (ui->srcStack->currentIndex()==STACK_TABLE)
        return ui->srcTable;

    return ui->srcList;
}

QModelIndexList ZSearchTab::getSelectedIndexes() const
{
    QModelIndexList res;
    const QModelIndexList list = activeView()->selectionModel()->selectedIndexes();
    for (const QModelIndex& idx : list) {
        auto proxy = qobject_cast<QAbstractProxyModel *>(activeView()->model());
        if (proxy!=nullptr)
            res.append(proxy->mapToSource(idx));
    }
    return res;
}

QModelIndex ZSearchTab::mapToSource(const QModelIndex& index)
{
    auto proxy = qobject_cast<QAbstractProxyModel *>(activeView()->model());
    if (proxy!=nullptr)
        return proxy->mapToSource(index);

    return QModelIndex();
}

void ZSearchTab::albumClicked(QListWidgetItem *item)
{
    if (item==nullptr) return;

    emit statusBarMsg(tr("Searching..."));

    ui->srcDesc->clear();

    if (model)
        model->deleteAllItems();
    emit dbGetFiles(item->text(),QString(),Z::Name,false);
}

void ZSearchTab::mangaSearch()
{
    emit statusBarMsg(tr("Searching..."));

    ui->srcDesc->clear();

    if (model)
        model->deleteAllItems();

    QString s = ui->srcEdit->text().trimmed();

    searchHistoryModel->appendHistoryItem(s);

    emit dbGetFiles(QString(),s,Z::Name,false);
}

void ZSearchTab::mangaSelectionChanged(const QModelIndex &current, const QModelIndex &)
{
    ui->srcDesc->clear();
    if (!current.isValid()) return;
    if (!model) return;

    int maxl = model->getItemsCount();
    int row = mapToSource(current).row();

    if (row>=maxl) return;

    ui->srcDesc->clear();

    int idx = row;
    const SQLMangaEntry m = model->getItem(idx);

    QFileInfo fi(m.filename);

    QString msg = QString(descTemplate).
            arg(m.name).arg(m.pagesCount).arg(formatSize(m.fileSize),m.album,m.fileMagic,
            m.fileDT.toString("yyyy-MM-dd"),m.addingDT.toString("yyyy-MM-dd"),fi.path());

    ui->srcDesc->setHtml(msg);
}

void ZSearchTab::mangaOpen(const QModelIndex &index)
{
    if (!index.isValid()) return;
    if (!model) return;

    int maxl = model->getItemsCount();

    int idx = mapToSource(index).row();
    if (idx>=maxl) return;

    QString filename = model->getItem(idx).filename;

    if (model->getItem(idx).fileMagic=="DYN")
        filename.prepend("#DYN#");

    emit mangaDblClick(filename);
}

void ZSearchTab::mangaAdd()
{
    QStringList fl = getOpenFileNamesD(this,tr("Add manga to index"),zg->savedIndexOpenDir);
    if (fl.isEmpty()) return;
    fl.removeDuplicates();
    QFileInfo fi(fl.first());
    zg->savedIndexOpenDir = fi.path();

    QDir d(fi.path());
    QString album = d.dirName();
    album = getAlbumNameToAdd(album,fl.count());
    if (album.isEmpty()) return;

    emit dbAddFiles(fl,album);
}

void ZSearchTab::mangaAddDir()
{
    QString fi = getExistingDirectoryD(this,tr("Add manga to index from directory"),
                                                    zg->savedIndexOpenDir);
    if (fi.isEmpty()) return;
    zg->savedIndexOpenDir = fi;

    QDir d(fi);
    d.setFilter(QDir::Files | QDir::Readable);
    d.setNameFilters(QStringList("*"));
    QStringList files;
    QDirIterator it(d, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
    while (it.hasNext())
        files << it.next();

    QString album = d.dirName();

    album = getAlbumNameToAdd(album,-1);
    if (album.isEmpty()) return;

    emit dbAddFiles(files,album);
}

void ZSearchTab::mangaDel()
{
    if (!model) return;
    auto ac = qobject_cast<QAction *>(sender());
    if (ac==nullptr) return;
    bool delFiles = (ac->data().toInt() == 2);

    QIntList dl;
    int cnt = zg->db->getAlbumsCount();
    // remove other selected columns
    const QModelIndexList lii = getSelectedIndexes();
    QModelIndexList li;
    for (const auto &i : lii) {
        if (i.column()==0)
            li << i;
    }

    ui->srcDesc->clear();

    for (const auto &i : qAsConst(li)) {
        if (i.row()>=0 && i.row()<model->getItemsCount())
            dl << model->getItem(i.row()).dbid;
    }

    emit dbDelFiles(dl,delFiles);

    if (cnt!=zg->db->getAlbumsCount())
        updateAlbumsList();
}

void ZSearchTab::imagesAddDir()
{
    QString fi = getExistingDirectoryD(this,tr("Compose images from directory as manga"),
                                                    zg->savedIndexOpenDir);
    if (fi.isEmpty()) return;
    zg->savedIndexOpenDir = fi;

    QDir d(fi);
    const QFileInfoList fl = filterSupportedImgFiles(d.entryInfoList(QStringList("*"), QDir::Files | QDir::Readable));
    QStringList files;
    for (const auto &i : fl)
        files << i.absoluteFilePath();

    if (files.isEmpty()) {
        QMessageBox::warning(this,tr("QManga"),tr("Supported image files not found in specified directory."));
        return;
    }
    QString album = getAlbumNameToAdd(QString(),-1);
    if (album.isEmpty()) return;

    files.clear();
    files << QString("###DYNAMIC###");
    files << fi;

    emit dbAddFiles(files,album);
}

void ZSearchTab::listModeChanged(bool)
{
    if (ui->srcModeIcon->isChecked()) {
        ui->srcStack->setCurrentIndex(STACK_ICONS);
        ui->srcList->setGridSize(gridSize(ui->srcIconSize->value()));
    } else {
        ui->srcStack->setCurrentIndex(STACK_TABLE);
    }
}

void ZSearchTab::iconSizeChanged(int ref)
{
    ui->srcList->setGridSize(gridSize(ref));
}

void ZSearchTab::dbAlbumsListUpdated()
{
    updateAlbumsList();
}

void ZSearchTab::dbAlbumsListReady(const QStringList &albums)
{
    cachedAlbums.clear();
    cachedAlbums.append(albums);
    ui->srcAlbums->clear();
    ui->srcAlbums->addItems(albums);
    if (model)
        model->deleteAllItems();
}

void ZSearchTab::dbFilesAdded(const int count, const int total, const int elapsed)
{
    emit statusBarMsg(QString("MBOX#Added %1 out of %2 found files in %3s").arg(count).arg(total).arg((double)elapsed/1000.0,1,'f',2));
}

void ZSearchTab::dbFilesLoaded(const int count, const int elapsed)
{
    emit statusBarMsg(QString("Found %1 results in %2s").arg(count).arg((double)elapsed/1000.0,1,'f',2));
    ui->srcTable->resizeColumnsToContents();
}

void ZSearchTab::dbErrorMsg(const QString &msg)
{
    QMessageBox::critical(this,tr("QManga"),msg);
}

void ZSearchTab::dbNeedTableCreation()
{
    if (QMessageBox::question(this,tr("QManga"),tr("Database is empty. Recreate tables and structures?"),
                              QMessageBox::Yes,QMessageBox::No)==QMessageBox::Yes)
        emit dbCreateTables();
}

void ZSearchTab::dbShowProgressDialog(const bool visible)
{
    dbShowProgressDialogEx(visible, QString());
}

void ZSearchTab::dbShowProgressDialogEx(const bool visible, const QString& title)
{
    if (visible) {
        if (progressDlg==nullptr) {
            progressDlg = new QProgressDialog(tr("Adding files"),tr("Cancel"),0,100,this);
            connect(progressDlg,&QProgressDialog::canceled,
                    zg->db,&ZDB::sqlCancelAdding,Qt::QueuedConnection);
        }
        progressDlg->setWindowModality(Qt::WindowModal);
        if (title.isEmpty())
            progressDlg->setWindowTitle(tr("Adding files to index..."));
        else
            progressDlg->setWindowTitle(title);
        progressDlg->setValue(0);
        progressDlg->setLabelText(QString());
        progressDlg->show();
    } else {
        progressDlg->hide();
    }
}

void ZSearchTab::dbShowProgressState(const int value, const QString &msg)
{
    if (progressDlg!=nullptr) {
        progressDlg->setValue(value);
        progressDlg->setLabelText(msg);
    }
}
