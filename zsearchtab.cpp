#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QProcess>
#include <QFileInfo>

#include "zsearchtab.h"
#include "ui_zsearchtab.h"
#include "zglobal.h"
#include "zmangamodel.h"
#include "albumselectordlg.h"

ZSearchTab::ZSearchTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ZSearchTab)
{
    ui->setupUi(this);

    cachedAlbums.clear();

    progressDlg.setParent(this,Qt::Dialog);
    progressDlg.setMinimum(0);
    progressDlg.setMaximum(100);
    progressDlg.setCancelButtonText(tr("Cancel"));
    progressDlg.setLabelText(tr("Adding files"));

    descTemplate = ui->srcDesc->toHtml();
    ui->srcDesc->clear();
    ui->srcLoading->hide();

    srclIconSize = ui->srcIconSize;
    srclModeIcon = ui->srcModeIcon;
    srclModeList = ui->srcModeList;

    ui->srcIconSize->setMinimum(16);
    ui->srcIconSize->setMaximum(maxPreviewSize);
    ui->srcIconSize->setValue(128);
    ui->srcList->setGridSize(gridSize(ui->srcIconSize->value()));
    ui->srcList->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    ui->srcModeIcon->setChecked(ui->srcList->viewMode()==QListView::IconMode);
    ui->srcModeList->setChecked(ui->srcList->viewMode()==QListView::ListMode);
    ui->srcAlbums->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->srcAlbums,SIGNAL(itemClicked(QListWidgetItem*)),this,SLOT(albumClicked(QListWidgetItem*)));
    connect(ui->srcAlbums,SIGNAL(customContextMenuRequested(QPoint)),
            this,SLOT(ctxAlbumMenu(QPoint)));
    connect(ui->srcList,SIGNAL(doubleClicked(QModelIndex)),this,SLOT(mangaOpen(QModelIndex)));
    connect(ui->srcList,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(ctxMenu(QPoint)));
    connect(ui->srcAddBtn,SIGNAL(clicked()),this,SLOT(mangaAdd()));
    connect(ui->srcAddDirBtn,SIGNAL(clicked()),this,SLOT(mangaAddDir()));
    connect(ui->srcDelBtn,SIGNAL(clicked()),this,SLOT(mangaDel()));
    connect(ui->srcModeIcon,SIGNAL(toggled(bool)),this,SLOT(listModeChanged(bool)));
    connect(ui->srcModeList,SIGNAL(toggled(bool)),this,SLOT(listModeChanged(bool)));
    connect(ui->srcIconSize,SIGNAL(valueChanged(int)),this,SLOT(iconSizeChanged(int)));
    connect(ui->srcEdit,SIGNAL(returnPressed()),ui->srcEditBtn,SLOT(click()));
    connect(ui->srcEditBtn,SIGNAL(clicked()),this,SLOT(mangaSearch()));

    connect(&progressDlg,SIGNAL(canceled()),
            zg->db,SLOT(sqlCancelAdding()),Qt::QueuedConnection);

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
    connect(this,SIGNAL(dbDelFiles(QIntList)),
            zg->db,SLOT(sqlDelFiles(QIntList)),Qt::QueuedConnection);
    connect(this,SIGNAL(dbGetAlbums()),
            zg->db,SLOT(sqlGetAlbums()),Qt::QueuedConnection);
    connect(this,SIGNAL(dbGetFiles(QString,QString,int,bool)),
            zg->db,SLOT(sqlGetFiles(QString,QString,int,bool)),Qt::QueuedConnection);
    connect(this,SIGNAL(dbRenameAlbum(QString,QString)),
            zg->db,SLOT(sqlRenameAlbum(QString,QString)),Qt::QueuedConnection);
    connect(this,SIGNAL(dbCreateTables()),
            zg->db,SLOT(sqlCreateTables()),Qt::QueuedConnection);
    connect(this,SIGNAL(dbDeleteAlbum(QString)),
            zg->db,SLOT(sqlDelAlbum(QString)),Qt::QueuedConnection);

    order = zg->defaultOrdering;
    reverseOrder = false;
    connect(ui->actionSortByName,SIGNAL(triggered()),this,SLOT(ctxSorting()));
    connect(ui->actionSortByAlbum,SIGNAL(triggered()),this,SLOT(ctxSorting()));
    connect(ui->actionSortByPageCount,SIGNAL(triggered()),this,SLOT(ctxSorting()));
    connect(ui->actionSortByAdded,SIGNAL(triggered()),this,SLOT(ctxSorting()));
    connect(ui->actionSortByCreation,SIGNAL(triggered()),this,SLOT(ctxSorting()));
    connect(ui->actionSortReverse,SIGNAL(triggered()),this,SLOT(ctxSorting()));
    ctxSorting();

    QState* sLoaded = new QState(); QState* sLoading = new QState();
    sLoading->addTransition(zg->db,SIGNAL(filesLoaded(int,int)),sLoaded);
    sLoaded->addTransition(this,SIGNAL(dbGetFiles(QString,QString,int,bool)),sLoading);
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

    ZMangaModel* aModel = new ZMangaModel(this,ui->srcIconSize,ui->srcList);
    ui->srcList->setModel(aModel);
    connect(ui->srcList->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this,SLOT(mangaSelectionChanged(QModelIndex,QModelIndex)));

    connect(zg->db,SIGNAL(gotFile(SQLMangaEntry)),
            aModel,SLOT(addItem(SQLMangaEntry)),Qt::QueuedConnection);
    connect(zg->db,SIGNAL(deleteItemsFromModel(QIntList)),
            aModel,SLOT(deleteItems(QIntList)),Qt::QueuedConnection);
    model = aModel;

    ui->srcList->setContextMenuPolicy(Qt::CustomContextMenu);

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
    QMenu cm(ui->srcList);
    QMenu* smenu = cm.addMenu(QIcon::fromTheme("view-sort-ascending"),tr("Sort"));

    smenu->addAction(ui->actionSortByName);
    smenu->addAction(ui->actionSortByAlbum);
    smenu->addAction(ui->actionSortByPageCount);
    smenu->addAction(ui->actionSortByAdded);
    smenu->addAction(ui->actionSortByCreation);
    smenu->addSeparator();
    smenu->addAction(ui->actionSortReverse);
    cm.addSeparator();

    QAction* acm;
    acm = cm.addAction(QIcon::fromTheme("edit-select-all"),tr("Select All"));
    connect(acm,SIGNAL(triggered()),ui->srcList,SLOT(selectAll()));

    cm.addSeparator();

    QModelIndexList li = ui->srcList->selectionModel()->selectedIndexes();
    int cnt = 0;
    for (int i=0;i<li.count();i++) {
        if (li.at(i).column()==0)
            cnt++;
    }
    acm = cm.addAction(QIcon::fromTheme("edit-delete"),
                       tr("Delete selected %1 files").arg(cnt),this,SLOT(mangaDel()));
    acm->setEnabled(cnt>0 && !ui->srcLoading->isVisible());

    acm = cm.addAction(QIcon::fromTheme("document-open-folder"),
                       tr("Open containing directory"),this,SLOT(ctxOpenDir()));
    acm->setEnabled(cnt>0 && !ui->srcLoading->isVisible());

    acm = cm.addAction(QIcon::fromTheme("fork"),
                       tr("Open with default DE action"),this,SLOT(ctxXdgOpen()));

    cm.exec(ui->srcList->mapToGlobal(pos));
}

void ZSearchTab::ctxAlbumMenu(QPoint pos)
{
    QListWidgetItem* itm = ui->srcAlbums->itemAt(pos);
    if (itm==NULL) return;

    QMenu cm(ui->srcAlbums);
    QAction* acm;
    acm = cm.addAction(QIcon::fromTheme("edit-rename"),tr("Rename album"),
                       this,SLOT(ctxRenameAlbum()));
    acm->setData(itm->text());
    acm = cm.addAction(QIcon::fromTheme("edit-delete"),tr("Delete album"),
                       this,SLOT(ctxDeleteAlbum()));
    acm->setData(itm->text());

    cm.exec(ui->srcAlbums->mapToGlobal(pos));
}

void ZSearchTab::ctxSorting()
{
    QAction* am = NULL;
    if (sender()!=NULL)
        am = qobject_cast<QAction *>(sender());

    Z::Ordering a = order;
    bool r = reverseOrder;
    if (am!=NULL) {
        if (am->text()==ui->actionSortByName->text())
            a = Z::FileName;
        else if (am->text()==ui->actionSortByAlbum->text())
            a = Z::Album;
        else if (am->text()==ui->actionSortByPageCount->text())
            a = Z::PagesCount;
        else if (am->text()==ui->actionSortByAdded->text())
            a = Z::AddingDate;
        else if (am->text()==ui->actionSortByCreation->text())
            a = Z::FileDate;
        else if (am->text()==ui->actionSortReverse->text())
            r = !r;
    }
    applyOrder(a,r,true);
}

void ZSearchTab::ctxRenameAlbum()
{
    QAction* nt = qobject_cast<QAction *>(sender());
    if (nt==NULL) return;
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
    if (nt==NULL) return;
    QString s = nt->data().toString();
    if (s.isEmpty()) return;

    if (QMessageBox::question(this,tr("Delete album"),tr("Are you sure to delete album '%1' and ALL it's contents?").arg(s),
                                      QMessageBox::Yes | QMessageBox::No, QMessageBox::No)==QMessageBox::Yes)
        emit dbDeleteAlbum(s);
}

void ZSearchTab::ctxOpenDir()
{
    QModelIndexList li;
    li.clear();
    QModelIndexList lii = ui->srcList->selectionModel()->selectedIndexes();
    if (lii.isEmpty()) return;

    for (int i=0;i<lii.count();i++) {
        if (lii.at(i).column()==0)
            li << lii.at(i);
    }

    QStringList edl;
    QStringList dl;
    edl.clear();
    dl.clear();
    for (int i=0;i<li.count();i++) {
        if (li.at(i).row()>=0 && li.at(i).row()<model->getItemsCount()) {
            QString fname = model->getItem(li.at(i).row()).filename;
            QFileInfo fi(fname);
            if (!fi.isFile() || !fi.exists())
                edl << fname;
            else {
                if (!dl.contains(fi.path()))
                    dl << fi.path();
            }
        }
    }

    for (int i=0;i<dl.count();i++) {
        QStringList params;
        params << dl.at(i);
        QProcess::startDetached("xdg-open",params);
    }

    if (!edl.isEmpty()) {
        QMessageBox::warning(this,tr("QManga"),
                             tr("Error while searching file path for some files.\n\n%1").
                             arg(edl.join("\n")));
    }
}

void ZSearchTab::ctxXdgOpen()
{
    QModelIndexList li;
    li.clear();
    QModelIndexList lii = ui->srcList->selectionModel()->selectedIndexes();
    if (lii.isEmpty()) return;

    for (int i=0;i<lii.count();i++) {
        if (lii.at(i).column()==0)
            li << lii.at(i);
    }

    QStringList dl;
    dl.clear();
    for (int i=0;i<li.count();i++) {
        if (li.at(i).row()>=0 && li.at(i).row()<model->getItemsCount()) {
            QString fname = model->getItem(li.at(i).row()).filename;
            QFileInfo fi(fname);
            if (fi.isFile() && fi.exists())
                dl << fname;
        }
    }

    for (int i=0;i<dl.count();i++) {
        QStringList params;
        params << dl.at(i);
        QProcess::startDetached("xdg-open",params);
    }

    if (dl.isEmpty()) {
        QMessageBox::warning(this,tr("QManga"),
                             tr("Error while searching file path for some files.\n\n%1").
                             arg(dl.join("\n")));
    }
}

void ZSearchTab::updateAlbumsList()
{
    emit dbGetAlbums();
}

void ZSearchTab::applyOrder(Z::Ordering aOrder, bool aReverseOrder, bool updateGUI)
{
    order = aOrder;
    reverseOrder = aReverseOrder;
    ui->actionSortByName->setChecked(order==Z::FileName);
    ui->actionSortByAlbum->setChecked(order==Z::Album);
    ui->actionSortByPageCount->setChecked(order==Z::PagesCount);
    ui->actionSortByAdded->setChecked(order==Z::AddingDate);
    ui->actionSortByCreation->setChecked(order==Z::FileDate);
    ui->actionSortReverse->setChecked(reverseOrder);

    if (updateGUI)
        albumClicked(ui->srcAlbums->currentItem());

    zg->defaultOrdering = order;
}

QSize ZSearchTab::gridSize(int ref)
{
    QFontMetrics fm(font());
    if (ui->srcList->viewMode()==QListView::IconMode)
        return QSize(ref+25,ref*previewProps+fm.height()*4);

    return QSize(ui->srcList->width()/3,25*fm.height()/10);
}

QString ZSearchTab::getAlbumNameToAdd(QString suggest, int toAddCount)
{
    AlbumSelectorDlg* dlg = new AlbumSelectorDlg(this,cachedAlbums,suggest,toAddCount);
    QString ret = QString();
    if (dlg->exec()) {
        ret = dlg->listAlbums->lineEdit()->text();
    }
    dlg->setParent(NULL);
    delete dlg;
    return ret;
}

void ZSearchTab::albumClicked(QListWidgetItem *item)
{
    if (item==NULL) return;

    emit statusBarMsg(tr("Searching..."));

    ui->srcDesc->clear();

    if (model)
        model->deleteAllItems();
    emit dbGetFiles(item->text(),QString(),(int)order,reverseOrder);
}

void ZSearchTab::mangaSearch()
{
    emit statusBarMsg(tr("Searching..."));

    ui->srcDesc->clear();

    if (model)
        model->deleteAllItems();
    emit dbGetFiles(QString(),ui->srcEdit->text(),(int)order,reverseOrder);
}

void ZSearchTab::mangaSelectionChanged(const QModelIndex &current, const QModelIndex &)
{
    ui->srcDesc->clear();
    if (!current.isValid()) return;
    if (!model) return;

    int maxl = model->getItemsCount();

    if (current.row()>=maxl) return;

    ui->srcDesc->clear();

    int idx = current.row();
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

    int idx = index.row();
    if (idx>=maxl) return;

    QString filename = model->getItem(idx).filename;

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
    QFileInfoList fl = d.entryInfoList(QStringList("*"));
    QStringList files;
    for (int i=0;i<fl.count();i++)
        if (fl.at(i).isReadable() && fl.at(i).isFile())
            files << fl.at(i).absoluteFilePath();
    QString album = d.dirName();

    album = getAlbumNameToAdd(album,-1);
    if (album.isEmpty()) return;

    emit dbAddFiles(files,album);
}

void ZSearchTab::mangaDel()
{
    if (!model) return;
    QIntList dl;
    int cnt = zg->db->getAlbumsCount();
    // remove other selected columns
    QModelIndexList lii = ui->srcList->selectionModel()->selectedIndexes();
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

    emit dbDelFiles(dl);

    if (cnt!=zg->db->getAlbumsCount())
        updateAlbumsList();
}

void ZSearchTab::listModeChanged(bool)
{
    if (ui->srcModeIcon->isChecked()) {
        ui->srcList->setViewMode(QListView::IconMode);
        ui->srcList->setGridSize(gridSize(ui->srcIconSize->value()));
    } else {
        ui->srcList->setViewMode(QListView::ListMode);
        ui->srcList->setGridSize(gridSize(0));
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
    emit statusBarMsg(QString("Added %1 out of %2 found files in %3s").arg(count).arg(total).arg((double)elapsed/1000.0,1,'f',2));
}

void ZSearchTab::dbFilesLoaded(const int count, const int elapsed)
{
    emit statusBarMsg(QString("Found %1 results in %2s").arg(count).arg((double)elapsed/1000.0,1,'f',2));
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
    if (visible) {
        progressDlg.setWindowModality(Qt::WindowModal);
        progressDlg.setWindowTitle(tr("Adding files to index..."));
        progressDlg.setValue(0);
        progressDlg.setLabelText(QString());
        progressDlg.show();
    } else {
        progressDlg.hide();
    }
}

void ZSearchTab::dbShowProgressState(const int value, const QString &msg)
{
    progressDlg.setValue(value);
    progressDlg.setLabelText(msg);
}
