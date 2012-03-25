#include "zsearchtab.h"
#include "ui_zsearchtab.h"
#include "zglobal.h"
#include "zmangamodel.h"

ZSearchTab::ZSearchTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ZSearchTab)
{
    ui->setupUi(this);

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

    connect(&loader,SIGNAL(finished()),this,SLOT(loaderFinished()));

    order = ZGlobal::FileName;
    reverseOrder = false;

    model = new ZMangaModel(this,&mangaList,ui->srcIconSize,ui->srcList,&listUpdating);
    ui->srcList->setModel(model);
    connect(ui->srcList->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this,SLOT(mangaSelectionChanged(QModelIndex,QModelIndex)));

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
    acm->setChecked(order==ZGlobal::FileName);
    connect(acm,SIGNAL(triggered()),this,SLOT(ctxSortName()));
    acm = smenu->addAction(tr("By album"));
    acm->setCheckable(true);
    acm->setChecked(order==ZGlobal::Album);
    connect(acm,SIGNAL(triggered()),this,SLOT(ctxSortAlbum()));
    acm = smenu->addAction(tr("By page count"));
    acm->setCheckable(true);
    acm->setChecked(order==ZGlobal::PagesCount);
    connect(acm,SIGNAL(triggered()),this,SLOT(ctxSortPage()));
    acm = smenu->addAction(tr("By added date"));
    acm->setCheckable(true);
    acm->setChecked(order==ZGlobal::AddingDate);
    connect(acm,SIGNAL(triggered()),this,SLOT(ctxSortAdded()));
    acm = smenu->addAction(tr("By file creation date"));
    acm->setCheckable(true);
    acm->setChecked(order==ZGlobal::FileDate);
    connect(acm,SIGNAL(triggered()),this,SLOT(ctxSortCreated()));
    smenu->addSeparator();
    acm = smenu->addAction(tr("Reverse order"));
    connect(acm,SIGNAL(triggered()),this,SLOT(ctxReverseOrder()));
    acm->setCheckable(true);
    acm->setChecked(reverseOrder);
    cm.addSeparator();
    int cnt = ui->srcList->selectionModel()->selectedIndexes().count();
    acm = cm.addAction(QIcon::fromTheme("edit-delete"),tr("Delete selected %1 files").arg(cnt),this,SLOT(mangaDel()));
    acm->setEnabled(cnt>0);

    cm.exec(ui->srcList->mapToGlobal(pos));
}

void ZSearchTab::albumCtxMenu(QPoint pos)
{
    QListWidgetItem* itm = ui->srcAlbums->itemAt(pos);
    if (itm==NULL) return;

    QMenu cm(ui->srcAlbums);
    QAction* acm;
    acm = cm.addAction(QIcon::fromTheme("edit-rename"),tr("Rename album"),this,SLOT(ctxRenameAlbum()));
    acm->setData(itm->text());

    cm.exec(ui->srcAlbums->mapToGlobal(pos));
}

void ZSearchTab::ctxSortName()
{
    order = ZGlobal::FileName;
    albumClicked(ui->srcAlbums->currentItem());
}

void ZSearchTab::ctxSortAlbum()
{
    order = ZGlobal::Album;
    albumClicked(ui->srcAlbums->currentItem());
}

void ZSearchTab::ctxSortPage()
{
    order = ZGlobal::PagesCount;
    albumClicked(ui->srcAlbums->currentItem());
}

void ZSearchTab::ctxSortAdded()
{
    order = ZGlobal::AddingDate;
    albumClicked(ui->srcAlbums->currentItem());
}

void ZSearchTab::ctxSortCreated()
{
    order = ZGlobal::FileDate;
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

    zGlobal->sqlRenameAlbum(s,n);

    updateAlbumsList();
}

void ZSearchTab::updateAlbumsList()
{
    ui->srcAlbums->clear();
    ui->srcAlbums->addItems(zGlobal->sqlGetAlbums());

    listUpdating.lock();
    int cnt = mangaList.count()-1;
    listUpdating.unlock();

    model->rowsAboutToDeleted(0,cnt);

    listUpdating.lock();
    mangaList.clear();
    listUpdating.unlock();

    model->rowsDeleted();
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

void ZSearchTab::albumChanged(QListWidgetItem *current, QListWidgetItem *)
{
    if (current==NULL) return;
    if (loadingNow) return;

    emit statusBarMsg(tr("Searching..."));

    ui->srcDesc->clear();

    loadingNow = true;
    updateWidgetsState();
    loader.setParams(&mangaList,&listUpdating,model,current->text(),QString(),order,reverseOrder);
    searchTimer.start();
    loader.start();
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
    loader.setParams(&mangaList,&listUpdating,model,QString(),ui->srcEdit->text(),order,reverseOrder);
    searchTimer.start();
    loader.start();
}

void ZSearchTab::mangaSelectionChanged(const QModelIndex &current, const QModelIndex &)
{
    ui->srcDesc->clear();
    if (!current.isValid()) return;

    listUpdating.lock();
    int maxl = mangaList.length();
    listUpdating.unlock();

    if (current.row()>=maxl) return;

    ui->srcDesc->clear();

    int idx = current.row();
    listUpdating.lock();
    const SQLMangaEntry m = mangaList.at(idx);
    listUpdating.unlock();

    QString msg = QString(descTemplate).
            arg(m.name).arg(m.pagesCount).arg(formatSize(m.fileSize)).arg(m.album).arg(m.fileMagic).
            arg(m.fileDT.toString("yyyy-MM-dd")).arg(m.addingDT.toString("yyyy-MM-dd"));

    ui->srcDesc->setHtml(msg);
}

void ZSearchTab::mangaOpen(const QModelIndex &index)
{
    if (!index.isValid()) return;

    listUpdating.lock();
    int maxl = mangaList.length();
    listUpdating.unlock();

    int idx = index.row();
    if (idx>=maxl) return;

    listUpdating.lock();
    QString filename = mangaList.at(idx).filename;
    listUpdating.unlock();

    emit mangaDblClick(filename);
}

void ZSearchTab::mangaAdd()
{
    if (loadingNow) return;
    QStringList fl = zGlobal->getOpenFileNamesD(this,tr("Add manga to index"),zGlobal->savedIndexOpenDir);
    if (fl.isEmpty()) return;
    fl.removeDuplicates();
    QFileInfo fi(fl.first());
    zGlobal->savedIndexOpenDir = fi.path();

    QDir d(fi.path());
    QString album = d.dirName();
    bool ok;
    album = QInputDialog::getText(this,tr("QManga"),tr("Album name"),QLineEdit::Normal,album,&ok);
    int cnt = 0;
    int el = 0;
    if (ok) {
        QTime tmr;
        tmr.start();
        cnt = zGlobal->sqlAddFiles(fl,album);
        el = tmr.elapsed();
    } else
        return;

    updateAlbumsList();

    emit statusBarMsg(QString("Added %1 out of %2 found files in %3s").arg(cnt).arg(fl.count()).arg((double)el/1000.0,1,'f',2));
}

void ZSearchTab::mangaAddDir()
{
    if (loadingNow) return;
    QString fi = zGlobal->getExistingDirectoryD(this,tr("Add manga to index from directory"),
                                                    zGlobal->savedIndexOpenDir);
    if (fi.isEmpty()) return;
    zGlobal->savedIndexOpenDir = fi;

    QDir d(fi);
    QFileInfoList fl = d.entryInfoList(QStringList("*"));
    QStringList files;
    for (int i=0;i<fl.count();i++)
        if (fl.at(i).isReadable() && fl.at(i).isFile())
            files << fl.at(i).absoluteFilePath();
    QString album = d.dirName();

    bool ok;
    album = QInputDialog::getText(this,tr("QManga"),tr("Album name"),QLineEdit::Normal,album,&ok);
    int cnt = 0;
    int el = 0;
    if (ok) {
        QTime tmr;
        tmr.start();
        cnt = zGlobal->sqlAddFiles(files,album);
        el = tmr.elapsed();
    } else
        return;

    updateAlbumsList();

    emit statusBarMsg(QString("Added %1 out of %2 found files in %3s").arg(cnt).arg(files.count()).arg((double)el/1000.0,1,'f',2));
}

void ZSearchTab::mangaDel()
{
    if (loadingNow) return;
    QIntList dl;
    QStringList albums = zGlobal->sqlGetAlbums();
    QModelIndexList li = ui->srcList->selectionModel()->selectedIndexes();

    ui->srcDesc->clear();

    listUpdating.lock();
    for (int i=0;i<li.count();i++) {
        if (li.at(i).row()>=0 && li.at(i).row()<mangaList.count())
            dl << mangaList.at(li.at(i).row()).dbid;
    }
    listUpdating.unlock();

    zGlobal->sqlDelFiles(dl);

    for (int i=0;i<dl.count();i++) {
        listUpdating.lock();
        int idx = mangaList.indexOf(SQLMangaEntry(dl.at(i)));
        listUpdating.unlock();
        if (idx>=0) {
            model->rowsAboutToDeleted(idx,idx+1);
            listUpdating.lock();
            mangaList.removeAt(idx);
            listUpdating.unlock();
            model->rowsDeleted();
        }
    }

    if (albums.count()!=zGlobal->sqlGetAlbums().count())
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

void ZSearchTab::loaderFinished()
{
    loadingNow = false;
    int el = searchTimer.elapsed();
    updateWidgetsState();
    listUpdating.lock();
    int cnt = mangaList.count();
    listUpdating.unlock();
    emit statusBarMsg(QString("Found %1 results in %2s").arg(cnt).arg((double)el/1000.0,1,'f',2));
}
