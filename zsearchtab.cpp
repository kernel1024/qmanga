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
    loadingNow = false;
    ui->srcLoading->hide();

    srclIconSize = ui->srcIconSize;
    srclModeIcon = ui->srcModeIcon;
    srclModeList = ui->srcModeList;

    ui->srcIconSize->setMinimum(16);
    ui->srcIconSize->setMaximum(maxPreviewSize);
    ui->srcIconSize->setValue(128);
    ui->srcList->setGridSize(gridSize(ui->srcIconSize->value()));
    ui->srcModeIcon->setChecked(ui->srcList->viewMode()==QListView::IconMode);
    ui->srcModeList->setChecked(ui->srcList->viewMode()==QListView::ListMode);
    ui->srcAlbums->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->srcAlbums,SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this,SLOT(albumChanged(QListWidgetItem*,QListWidgetItem*)));
    connect(ui->srcAlbums,SIGNAL(itemActivated(QListWidgetItem*)),this,SLOT(albumClicked(QListWidgetItem*)));
    connect(ui->srcAlbums,SIGNAL(customContextMenuRequested(QPoint)),
            this,SLOT(albumCtxMenu(QPoint)));
    connect(ui->srcList,SIGNAL(activated(QModelIndex)),this,SLOT(mangaOpen(QModelIndex)));
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
            this,SLOT(showProgressDialog(bool)),Qt::QueuedConnection);
    connect(zg->db,SIGNAL(showProgressState(int,QString)),
            this,SLOT(showProgressState(int,QString)),Qt::QueuedConnection);

    connect(this,SIGNAL(dbAddFiles(QStringList,QString)),
            zg->db,SLOT(sqlAddFiles(QStringList,QString)),Qt::QueuedConnection);
    connect(this,SIGNAL(dbDelFiles(QIntList)),
            zg->db,SLOT(sqlDelFiles(QIntList)),Qt::QueuedConnection);
    connect(this,SIGNAL(dbClearList()),
            zg->db,SLOT(sqlClearList()),Qt::QueuedConnection);
    connect(this,SIGNAL(dbGetAlbums()),
            zg->db,SLOT(sqlGetAlbums()),Qt::QueuedConnection);
    connect(this,SIGNAL(dbGetFiles(QString,QString,int,bool)),
            zg->db,SLOT(sqlGetFiles(QString,QString,int,bool)),Qt::QueuedConnection);
    connect(this,SIGNAL(dbRenameAlbum(QString,QString)),
            zg->db,SLOT(sqlRenameAlbum(QString,QString)),Qt::QueuedConnection);
    connect(this,SIGNAL(dbCreateTables()),
            zg->db,SLOT(sqlCreateTables()),Qt::QueuedConnection);


    order = Z::FileName;
    reverseOrder = false;

    model = new ZMangaModel(this,ui->srcIconSize,ui->srcList);
    ui->srcList->setModel(model);
    connect(ui->srcList->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this,SLOT(mangaSelectionChanged(QModelIndex,QModelIndex)));

    connect(zg->db,SIGNAL(listRowsAboutToAppended(int,int)),
            model,SLOT(rowsAboutToAppended(int,int)),Qt::QueuedConnection);
    connect(zg->db,SIGNAL(listRowsAppended()),model,SLOT(rowsAppended()),Qt::QueuedConnection);
    connect(zg->db,SIGNAL(listRowsAboutToDeleted(int,int)),
            model,SLOT(rowsAboutToDeleted(int,int)),Qt::QueuedConnection);
    connect(zg->db,SIGNAL(listRowsDeleted()),model,SLOT(rowsDeleted()),Qt::QueuedConnection);

    ui->srcList->setContextMenuPolicy(Qt::CustomContextMenu);

    updateSplitters();
    updateWidgetsState();
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
    QAction* acm;
    acm = smenu->addAction(tr("By name"));
    acm->setCheckable(true);
    acm->setChecked(order==Z::FileName);
    connect(acm,SIGNAL(triggered()),this,SLOT(ctxSortName()));
    acm = smenu->addAction(tr("By album"));
    acm->setCheckable(true);
    acm->setChecked(order==Z::Album);
    connect(acm,SIGNAL(triggered()),this,SLOT(ctxSortAlbum()));
    acm = smenu->addAction(tr("By page count"));
    acm->setCheckable(true);
    acm->setChecked(order==Z::PagesCount);
    connect(acm,SIGNAL(triggered()),this,SLOT(ctxSortPage()));
    acm = smenu->addAction(tr("By added date"));
    acm->setCheckable(true);
    acm->setChecked(order==Z::AddingDate);
    connect(acm,SIGNAL(triggered()),this,SLOT(ctxSortAdded()));
    acm = smenu->addAction(tr("By file creation date"));
    acm->setCheckable(true);
    acm->setChecked(order==Z::FileDate);
    connect(acm,SIGNAL(triggered()),this,SLOT(ctxSortCreated()));
    smenu->addSeparator();
    acm = smenu->addAction(tr("Reverse order"));
    connect(acm,SIGNAL(triggered()),this,SLOT(ctxReverseOrder()));
    acm->setCheckable(true);
    acm->setChecked(reverseOrder);
    cm.addSeparator();
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
    acm->setEnabled(cnt>0);

    cm.exec(ui->srcList->mapToGlobal(pos));
}

void ZSearchTab::albumCtxMenu(QPoint pos)
{
    QListWidgetItem* itm = ui->srcAlbums->itemAt(pos);
    if (itm==NULL) return;

    QMenu cm(ui->srcAlbums);
    QAction* acm;
    acm = cm.addAction(QIcon::fromTheme("edit-rename"),tr("Rename album"),
                       this,SLOT(ctxRenameAlbum()));
    acm->setData(itm->text());

    cm.exec(ui->srcAlbums->mapToGlobal(pos));
}

void ZSearchTab::ctxSortName()
{
    order = Z::FileName;
    albumClicked(ui->srcAlbums->currentItem());
}

void ZSearchTab::ctxSortAlbum()
{
    order = Z::Album;
    albumClicked(ui->srcAlbums->currentItem());
}

void ZSearchTab::ctxSortPage()
{
    order = Z::PagesCount;
    albumClicked(ui->srcAlbums->currentItem());
}

void ZSearchTab::ctxSortAdded()
{
    order = Z::AddingDate;
    albumClicked(ui->srcAlbums->currentItem());
}

void ZSearchTab::ctxSortCreated()
{
    order = Z::FileDate;
    albumClicked(ui->srcAlbums->currentItem());
}

void ZSearchTab::ctxReverseOrder()
{
    reverseOrder = !reverseOrder;
    albumClicked(ui->srcAlbums->currentItem());
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

void ZSearchTab::showProgressDialog(const bool visible)
{
    if (visible) {
        progressDlg.setWindowModality(Qt::WindowModal);
        progressDlg.setWindowTitle(tr("Adding files to index..."));
        progressDlg.setValue(0);
        progressDlg.show();
    } else {
        progressDlg.hide();
    }
}

void ZSearchTab::showProgressState(const int value, const QString &msg)
{
    progressDlg.setValue(value);
    progressDlg.setLabelText(msg);
}

void ZSearchTab::dbAlbumsListUpdated()
{
    updateAlbumsList();
}

void ZSearchTab::dbAlbumsListReady(const QStringList &albums)
{
    emit dbClearList();
    cachedAlbums.clear();
    cachedAlbums.append(albums);
    ui->srcAlbums->clear();
    ui->srcAlbums->addItems(albums);
}

void ZSearchTab::dbFilesAdded(const int count, const int total, const int elapsed)
{
    emit statusBarMsg(QString("Added %1 out of %2 found files in %3s").arg(count).arg(total).arg((double)elapsed/1000.0,1,'f',2));
}

void ZSearchTab::updateAlbumsList()
{
    emit dbGetAlbums();
}

void ZSearchTab::updateWidgetsState()
{
    ui->srcAddBtn->setEnabled(!loadingNow);
    ui->srcAddDirBtn->setEnabled(!loadingNow);
    ui->srcDelBtn->setEnabled(!loadingNow);
    ui->srcEditBtn->setEnabled(!loadingNow);
    ui->srcEdit->setEnabled(!loadingNow);
    ui->srcAlbums->setEnabled(!loadingNow);
    ui->srcLoading->setVisible(loadingNow);
}

QSize ZSearchTab::gridSize(int ref)
{
    QFontMetrics fm(font());
    if (ui->srcList->viewMode()==QListView::IconMode)
        return QSize(ref+25,ref*previewProps+fm.height()*4);

    return QSize(ui->srcList->width()/3,25*fm.height()/10);
}

QString ZSearchTab::getAlbumNameToAdd(QString suggest)
{
    AlbumSelectorDlg* dlg = new AlbumSelectorDlg(this,cachedAlbums,suggest);
    QString ret = QString();
    if (dlg->exec()) {
        ret = dlg->listAlbums->lineEdit()->text();
    }
    dlg->setParent(NULL);
    delete dlg;
    return ret;
}

void ZSearchTab::albumChanged(QListWidgetItem *current, QListWidgetItem *)
{
    if (current==NULL) return;
    if (loadingNow) return;

    emit statusBarMsg(tr("Searching..."));

    ui->srcDesc->clear();

    loadingNow = true;
    updateWidgetsState();
    emit dbGetFiles(current->text(),QString(),(int)order,reverseOrder);
}

void ZSearchTab::albumClicked(QListWidgetItem *item)
{
    albumChanged(item,NULL);
}

void ZSearchTab::mangaSearch()
{
    if (loadingNow) return;

    emit statusBarMsg(tr("Searching..."));

    ui->srcDesc->clear();

    loadingNow = true;
    updateWidgetsState();
    emit dbGetFiles(QString(),ui->srcEdit->text(),(int)order,reverseOrder);
}

void ZSearchTab::mangaSelectionChanged(const QModelIndex &current, const QModelIndex &)
{
    ui->srcDesc->clear();
    if (!current.isValid()) return;

    int maxl = zg->db->getResultCount();

    if (current.row()>=maxl) return;

    ui->srcDesc->clear();

    int idx = current.row();
    const SQLMangaEntry m = zg->db->getResultItem(idx);

    QString msg = QString(descTemplate).
            arg(m.name).arg(m.pagesCount).arg(formatSize(m.fileSize)).arg(m.album).arg(m.fileMagic).
            arg(m.fileDT.toString("yyyy-MM-dd")).arg(m.addingDT.toString("yyyy-MM-dd"));

    ui->srcDesc->setHtml(msg);
}

void ZSearchTab::mangaOpen(const QModelIndex &index)
{
    if (!index.isValid()) return;

    int maxl = zg->db->getResultCount();

    int idx = index.row();
    if (idx>=maxl) return;

    QString filename = zg->db->getResultItem(idx).filename;

    emit mangaDblClick(filename);
}

void ZSearchTab::mangaAdd()
{
    if (loadingNow) return;
    QStringList fl = getOpenFileNamesD(this,tr("Add manga to index"),zg->savedIndexOpenDir);
    if (fl.isEmpty()) return;
    fl.removeDuplicates();
    QFileInfo fi(fl.first());
    zg->savedIndexOpenDir = fi.path();

    QDir d(fi.path());
    QString album = d.dirName();
    album = getAlbumNameToAdd(album);
    if (album.isEmpty()) return;

    emit dbAddFiles(fl,album);
}

void ZSearchTab::mangaAddDir()
{
    if (loadingNow) return;
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

    album = getAlbumNameToAdd(album);
    if (album.isEmpty()) return;

    emit dbAddFiles(files,album);
}

void ZSearchTab::mangaDel()
{
    if (loadingNow) return;
    QIntList dl;
    int cnt = zg->db->getAlbumsCount();
    QModelIndexList lii = ui->srcList->selectionModel()->selectedIndexes();
    QModelIndexList li;
    for (int i=0;i<lii.count();i++) {
        if (lii.at(i).column()==0)
            li << lii.at(i);
    }

    ui->srcDesc->clear();

    for (int i=0;i<li.count();i++) {
        if (li.at(i).row()>=0 && li.at(i).row()<zg->db->getResultCount())
            dl << zg->db->getResultItem(li.at(i).row()).dbid;
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

void ZSearchTab::dbFilesLoaded(const int count, const int elapsed)
{
    loadingNow = false;
    updateWidgetsState();
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
