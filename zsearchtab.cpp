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

    QCompleter *completer = new QCompleter(this);
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

    connect(ui->srcAlbums,SIGNAL(itemClicked(QListWidgetItem*)),this,SLOT(albumClicked(QListWidgetItem*)));
    connect(ui->srcAlbums,SIGNAL(customContextMenuRequested(QPoint)),
            this,SLOT(ctxAlbumMenu(QPoint)));
    connect(ui->srcList,SIGNAL(doubleClicked(QModelIndex)),this,SLOT(mangaOpen(QModelIndex)));
    connect(ui->srcList,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(ctxMenu(QPoint)));
    connect(ui->srcTable,SIGNAL(doubleClicked(QModelIndex)),this,SLOT(mangaOpen(QModelIndex)));
    connect(ui->srcTable,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(ctxMenu(QPoint)));
    connect(ui->srcAddBtn,SIGNAL(clicked()),this,SLOT(mangaAdd()));
    connect(ui->srcAddDirBtn,SIGNAL(clicked()),this,SLOT(mangaAddDir()));
    connect(ui->srcAddImgDirBtn,SIGNAL(clicked()),this,SLOT(imagesAddDir()));
    connect(ui->srcDelBtn,SIGNAL(clicked()),this,SLOT(mangaDel()));
    connect(ui->srcModeIcon,SIGNAL(toggled(bool)),this,SLOT(listModeChanged(bool)));
    connect(ui->srcModeList,SIGNAL(toggled(bool)),this,SLOT(listModeChanged(bool)));
    connect(ui->srcIconSize,SIGNAL(valueChanged(int)),this,SLOT(iconSizeChanged(int)));
    connect(ui->srcEdit,SIGNAL(returnPressed()),ui->srcEditBtn,SLOT(click()));
    connect(ui->srcEditBtn,SIGNAL(clicked()),this,SLOT(mangaSearch()));

    connect(zg->db,SIGNAL(albumsListUpdated()),
            this,SLOT(dbAlbumsListUpdated()),Qt::QueuedConnection);
    connect(zg->db,SIGNAL(gotAlbums(QStringList)),
            this,SLOT(dbAlbumsListReady(QStringList)),Qt::QueuedConnection);
    connect(this,SIGNAL(dbRenameAlbum(QString,QString)),
            zg->db,SLOT(sqlRenameAlbum(QString,QString)),Qt::QueuedConnection);
    connect(zg->db,SIGNAL(filesAdded(int,int,int)),
            this,SLOT(dbFilesAdded(int,int,int)),Qt::QueuedConnection);
    connect(zg->db,SIGNAL(filesLoaded(int,int)),
            this,SLOT(dbFilesLoaded(int,int)),Qt::QueuedConnection);
    connect(zg->db,SIGNAL(errorMsg(QString)),
            this,SLOT(dbErrorMsg(QString)),Qt::QueuedConnection);
    connect(zg->db,SIGNAL(needTableCreation()),
            this,SLOT(dbNeedTableCreation()),Qt::QueuedConnection);
    connect(zg->db,SIGNAL(showProgressDialog(bool)),
            this,SLOT(dbShowProgressDialog(bool)),Qt::QueuedConnection);
    connect(zg->db,SIGNAL(showProgressState(int,QString)),
            this,SLOT(dbShowProgressState(int,QString)),Qt::QueuedConnection);

    connect(this,SIGNAL(dbAddFiles(QStringList,QString)),
            zg->db,SLOT(sqlAddFiles(QStringList,QString)),Qt::QueuedConnection);
    connect(this,SIGNAL(dbDelFiles(QIntList,bool)),
            zg->db,SLOT(sqlDelFiles(QIntList,bool)),Qt::QueuedConnection);
    connect(this,SIGNAL(dbGetAlbums()),
            zg->db,SLOT(sqlGetAlbums()),Qt::QueuedConnection);
    connect(this,SIGNAL(dbGetFiles(QString,QString,Z::Ordering,bool)),
            zg->db,SLOT(sqlGetFiles(QString,QString,Z::Ordering,bool)),Qt::QueuedConnection);
    connect(this,SIGNAL(dbRenameAlbum(QString,QString)),
            zg->db,SLOT(sqlRenameAlbum(QString,QString)),Qt::QueuedConnection);
    connect(this,SIGNAL(dbCreateTables()),
            zg->db,SLOT(sqlCreateTables()),Qt::QueuedConnection);
    connect(this,SIGNAL(dbDeleteAlbum(QString)),
            zg->db,SLOT(sqlDelAlbum(QString)),Qt::QueuedConnection);

    QState* sLoaded = new QState(); QState* sLoading = new QState();
    sLoading->addTransition(zg->db,SIGNAL(filesLoaded(int,int)),sLoaded);
    sLoaded->addTransition(this,SIGNAL(dbGetFiles(QString,QString,Z::Ordering,bool)),sLoading);
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

    ZMangaModel* aModel = new ZMangaModel(this,ui->srcIconSize,ui->srcTable);
    tableModel = new ZMangaTableModel(this,ui->srcTable);
    iconModel = new ZMangaIconModel(this,ui->srcList);
    tableModel->setSourceModel(aModel);
    iconModel->setSourceModel(aModel);
    ui->srcList->setModel(iconModel);
    ui->srcTable->setModel(tableModel);
    connect(ui->srcTable->horizontalHeader(),&QHeaderView::sortIndicatorChanged,[this](int logicalIndex, Qt::SortOrder order){
        iconModel->sort(logicalIndex,order);
    });

    connect(ui->srcList->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this,SLOT(mangaSelectionChanged(QModelIndex,QModelIndex)));
    connect(ui->srcTable->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this,SLOT(mangaSelectionChanged(QModelIndex,QModelIndex)));

    connect(zg->db,SIGNAL(gotFile(SQLMangaEntry,Z::Ordering,bool)),
            aModel,SLOT(addItem(SQLMangaEntry,Z::Ordering,bool)));
    connect(zg->db,SIGNAL(deleteItemsFromModel(QIntList)),
            aModel,SLOT(deleteItems(QIntList)),Qt::QueuedConnection);
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

void ZSearchTab::ctxMenu(QPoint pos)
{
    QAbstractItemView *view = activeView();
    QMenu cm(view);

    QAction* acm;
    acm = cm.addAction(QIcon(":/16/edit-select-all"),tr("Select All"));
    connect(acm,SIGNAL(triggered()),view,SLOT(selectAll()));

    cm.addSeparator();

    QModelIndexList li = getSelectedIndexes();
    int cnt = 0;
    for (int i=0;i<li.count();i++) {
        if (li.at(i).column()==0)
            cnt++;
    }
    QMenu* dmenu = cm.addMenu(QIcon(":/16/edit-delete"),
                              tr("Delete selected %1 files").arg(cnt));
    acm = dmenu->addAction(tr("Delete only from database"),this,SLOT(mangaDel()));
    acm->setEnabled(cnt>0 && !ui->srcLoading->isVisible());
    acm->setData(1);
    acm = dmenu->addAction(tr("Delete from database and filesystem"),this,SLOT(mangaDel()));
    acm->setEnabled(cnt>0 && !ui->srcLoading->isVisible());
    acm->setData(2);

    acm = cm.addAction(QIcon(":/16/document-open-folder"),
                       tr("Open containing directory"),this,SLOT(ctxOpenDir()));
    acm->setEnabled(cnt>0 && !ui->srcLoading->isVisible());

    cm.addAction(QIcon(":/16/fork"),
                 tr("Open with default DE action"),this,SLOT(ctxXdgOpen()));

    cm.addSeparator();

#ifndef Q_OS_WIN
    cm.addAction(QIcon(":/16/edit-copy"),
                 tr("Copy files"),this,SLOT(ctxFileCopyClipboard()));
#endif

    cm.addAction(QIcon(":/16/edit-copy"),
                 tr("Copy files to directory..."),this,SLOT(ctxFileCopy()));

    if (li.count()==1) {
        SQLMangaEntry m = model->getItem(li.first().row());
        if (m.fileMagic == "PDF") {
            cm.addSeparator();
            dmenu = cm.addMenu(tr("Preferred PDF rendering"));

            acm = dmenu->addAction(tr("Autodetect"));
            acm->setCheckable(true);
            acm->setChecked(m.rendering==Z::PDFRendering::Autodetect);
            acm->setData(static_cast<int>(Z::PDFRendering::Autodetect));
            connect(acm,SIGNAL(triggered()),this,SLOT(ctxChangeRenderer()));
            acm = dmenu->addAction(tr("Force rendering mode"));
            acm->setCheckable(true);
            acm->setChecked(m.rendering==Z::PDFRendering::PageRenderer);
            acm->setData(static_cast<int>(Z::PDFRendering::PageRenderer));
            connect(acm,SIGNAL(triggered()),this,SLOT(ctxChangeRenderer()));
            acm = dmenu->addAction(tr("Force catalog mode"));
            acm->setCheckable(true);
            acm->setChecked(m.rendering==Z::PDFRendering::ImageCatalog);
            acm->setData(static_cast<int>(Z::PDFRendering::ImageCatalog));
            connect(acm,SIGNAL(triggered()),this,SLOT(ctxChangeRenderer()));
        }
    }

    cm.exec(view->mapToGlobal(pos));
}

void ZSearchTab::ctxChangeRenderer()
{
    QAction* am = qobject_cast<QAction *>(sender());
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

void ZSearchTab::ctxAlbumMenu(QPoint pos)
{
    QListWidgetItem* itm = ui->srcAlbums->itemAt(pos);
    if (itm==nullptr) return;

    QMenu cm(ui->srcAlbums);
    QAction* acm;
    acm = cm.addAction(QIcon(":/16/edit-rename"),tr("Rename album"),
                       this,SLOT(ctxRenameAlbum()));
    acm->setData(itm->text());
    acm = cm.addAction(QIcon(":/16/edit-delete"),tr("Delete album"),
                       this,SLOT(ctxDeleteAlbum()));
    acm->setData(itm->text());

    cm.exec(ui->srcAlbums->mapToGlobal(pos));
}

void ZSearchTab::ctxRenameAlbum()
{
    QAction* nt = qobject_cast<QAction *>(sender());
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
    QAction* nt = qobject_cast<QAction *>(sender());
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
    QFileInfoList fl = getSelectedMangaEntries(true);
    if (fl.isEmpty()) return;

    for (int i=0;i<fl.count();i++)
        QDesktopServices::openUrl(QUrl::fromLocalFile(fl.at(i).path()));

    if (fl.isEmpty())
        QMessageBox::warning(this,tr("QManga"),
                             tr("Error while searching file path for some files."));
}

void ZSearchTab::ctxXdgOpen()
{
    QFileInfoList fl = getSelectedMangaEntries(true);
    if (fl.isEmpty()) return;

    for (int i=0;i<fl.count();i++)
        QDesktopServices::openUrl(QUrl::fromLocalFile(fl.at(i).filePath()));

    if (fl.isEmpty())
        QMessageBox::warning(this,tr("QManga"),
                             tr("Error while searching file path for some files."));
}

void ZSearchTab::ctxFileCopyClipboard()
{
    QFileInfoList fl = getSelectedMangaEntries(true);
    if (fl.isEmpty()) return;

    QByteArray uriList;
    QByteArray plainList;

    foreach (const QFileInfo& fi, fl) {
        QByteArray ba = QUrl::fromLocalFile(fi.filePath()).toEncoded();
        if (!ba.isEmpty()) {
            uriList.append(ba); uriList.append('\x0D'); uriList.append('\x0A');
            plainList.append(ba); plainList.append('\x0A');
        }
    }

    if (uriList.isEmpty()) return;

    QMimeData *data = new QMimeData();
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

    QProgressDialog *dlg = new QProgressDialog(this);
    dlg->setWindowModality(Qt::WindowModal);
    dlg->setWindowTitle(tr("Copying manga files..."));

    ZFileCopier *zfc = new ZFileCopier(fl,dlg,dst);
    QThread *zfc_thread = new QThread();

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

void ZSearchTab::setListViewOptions(const QListView::ViewMode mode, const int iconSize)
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
    else
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
        sl = settings.value("search_history",QStringList()).value<QStringList>();
    }

    searchHistoryModel->setHistoryItems(sl);
}

void ZSearchTab::saveSearchItems(QSettings &settings)
{
    if (searchHistoryModel==nullptr) return;

    settings.setValue("search_history",QVariant::fromValue(searchHistoryModel->getHistoryItems()));
}

void ZSearchTab::applySortOrder(const Z::Ordering order)
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

QString ZSearchTab::getAlbumNameToAdd(QString suggest, int toAddCount)
{
    AlbumSelectorDlg* dlg = new AlbumSelectorDlg(this,cachedAlbums,suggest,toAddCount);
    QString ret = QString();
    if (dlg->exec()) {
        ret = dlg->listAlbums->lineEdit()->text();
    }
    dlg->setParent(nullptr);
    delete dlg;
    return ret;
}

QFileInfoList ZSearchTab::getSelectedMangaEntries(bool includeDirs)
{
    QFileInfoList res;
    QModelIndexList lii = getSelectedIndexes();
    if (lii.isEmpty()) return res;

    QModelIndexList li;
    for (int i=0;i<lii.count();i++) {
        if (lii.at(i).column()==0)
            li << lii.at(i);
    }

    for (int i=0;i<li.count();i++) {
        if (li.at(i).row()>=0 && li.at(i).row()<model->getItemsCount()) {
            QString fname = model->getItem(li.at(i).row()).filename;
            QFileInfo fi(fname);
            if (fi.exists() && ((fi.isDir() && includeDirs) || fi.isFile())) {
                if (!res.contains(fi))
                    res << fi;
            }
        }
    }

    return res;
}

QAbstractItemView *ZSearchTab::activeView()
{
    if (ui->srcStack->currentIndex()==STACK_TABLE)
        return ui->srcTable;
    else
        return ui->srcList;
}

QModelIndexList ZSearchTab::getSelectedIndexes()
{
    QModelIndexList res;
    foreach (const QModelIndex& idx, activeView()->selectionModel()->selectedIndexes()) {
        QAbstractProxyModel* proxy = qobject_cast<QAbstractProxyModel *>(activeView()->model());
        if (proxy!=nullptr)
            res.append(proxy->mapToSource(idx));
    }
    return res;
}

QModelIndex ZSearchTab::mapToSource(const QModelIndex& index)
{
    QAbstractProxyModel* proxy = qobject_cast<QAbstractProxyModel *>(activeView()->model());
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
            arg(m.name).arg(m.pagesCount).arg(formatSize(m.fileSize)).arg(m.album).arg(m.fileMagic).
            arg(m.fileDT.toString("yyyy-MM-dd")).arg(m.addingDT.toString("yyyy-MM-dd")).arg(fi.path());

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
    QAction *ac = qobject_cast<QAction *>(sender());
    if (ac==nullptr) return;
    bool delFiles = (ac->data().toInt() == 2);

    QIntList dl;
    int cnt = zg->db->getAlbumsCount();
    // remove other selected columns
    QModelIndexList lii = getSelectedIndexes();
    QModelIndexList li;
    for (int i=0;i<lii.count();i++) {
        if (lii.at(i).column()==0)
            li << lii.at(i);
    }

    ui->srcDesc->clear();

    for (int i=0;i<li.count();i++) {
        if (li.at(i).row()>=0 && li.at(i).row()<model->getItemsCount())
            dl << model->getItem(li.at(i).row()).dbid;
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
    QFileInfoList fl = d.entryInfoList(QStringList("*"), QDir::Files | QDir::Readable);
    filterSupportedImgFiles(fl);
    QStringList files;
    for (int i=0;i<fl.count();i++)
        files << fl.at(i).absoluteFilePath();

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

void ZSearchTab::dbShowProgressDialog(const bool visible, const QString& title)
{
    if (visible) {
        if (progressDlg==nullptr) {
            progressDlg = new QProgressDialog(tr("Adding files"),tr("Cancel"),0,100,this);
            connect(progressDlg,SIGNAL(canceled()),
                    zg->db,SLOT(sqlCancelAdding()),Qt::QueuedConnection);
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
