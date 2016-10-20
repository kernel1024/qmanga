#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QProcess>
#include <QFileInfo>
#include <QCompleter>

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

    progressDlg = NULL;

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

    sortActions.clear();
    order = zg->defaultOrdering;
    reverseOrder = false;
    for (int i=0;i<=Z::maxOrdering;i++) {
        QAction* ac;
        if (i<Z::maxOrdering)
            ac = new QAction(Z::sortMenu.value((Z::Ordering)i),this);
        else
            ac = new QAction(tr("Reverse order"),this);
        ac->setCheckable(true);
        ac->setChecked(false);
        connect(ac, &QAction::triggered, this, &ZSearchTab::ctxSorting);
        ac->setData(i);
        sortActions << ac;
    }
    ctxSorting();

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

    ZMangaModel* aModel = new ZMangaModel(this,ui->srcIconSize,ui->srcList);
    ui->srcList->setModel(aModel);
    ui->srcList->setItemDelegate(new ZMangaListItemDelegate(this,ui->srcList,aModel));
    ui->srcList->header->setSectionResizeMode(0,QHeaderView::Stretch);
    connect(ui->srcList->header, SIGNAL(sectionClicked(int)),
            this, SLOT(headerClicked(int)));

    connect(ui->srcList->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this,SLOT(mangaSelectionChanged(QModelIndex,QModelIndex)));

    connect(zg->db,SIGNAL(gotFile(SQLMangaEntry,Z::Ordering,bool)),
            aModel,SLOT(addItem(SQLMangaEntry,Z::Ordering,bool)));
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

void ZSearchTab::headerClicked(int logicalIndex)
{
    int s = (int)order;
    bool r = reverseOrder;

    if (s==logicalIndex)
        r = !r;
    else if (logicalIndex>=0 && logicalIndex<Z::maxOrdering)
        s = logicalIndex;

    applyOrder((Z::Ordering)s,r,true);
}

void ZSearchTab::ctxMenu(QPoint pos)
{
    QMenu cm(ui->srcList);
    QMenu* smenu = cm.addMenu(QIcon::fromTheme("view-sort-ascending"),tr("Sort"));

    for (int i=0;i<sortActions.count();i++) {
        if (i==(sortActions.count()-1))
            smenu->addSeparator();
        smenu->addAction(sortActions.at(i));
    }
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
    QMenu* dmenu = cm.addMenu(QIcon::fromTheme("edit-delete"),
                              tr("Delete selected %1 files").arg(cnt));
    acm = dmenu->addAction(tr("Delete only from database"),this,SLOT(mangaDel()));
    acm->setEnabled(cnt>0 && !ui->srcLoading->isVisible());
    acm->setData(1);
    acm = dmenu->addAction(tr("Delete from database and filesystem"),this,SLOT(mangaDel()));
    acm->setEnabled(cnt>0 && !ui->srcLoading->isVisible());
    acm->setData(2);

    acm = cm.addAction(QIcon::fromTheme("document-open-folder"),
                       tr("Open containing directory"),this,SLOT(ctxOpenDir()));
    acm->setEnabled(cnt>0 && !ui->srcLoading->isVisible());

    cm.addAction(QIcon::fromTheme("fork"),
                 tr("Open with default DE action"),this,SLOT(ctxXdgOpen()));

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

    cm.exec(ui->srcList->mapToGlobal(pos));
}

void ZSearchTab::ctxChangeRenderer()
{
    QAction* am = qobject_cast<QAction *>(sender());
    QModelIndexList li = ui->srcList->selectionModel()->selectedIndexes();
    if (am==NULL || li.count()!=1) return;

    SQLMangaEntry m = model->getItem(li.first().row());

    bool ok;
    Z::PDFRendering mode = static_cast<Z::PDFRendering>(am->data().toInt(&ok));
    if (ok)
        ok = zg->db->setPreferredRendering(m.filename, mode);

    albumClicked(ui->srcAlbums->currentItem());

    if (!ok)
        QMessageBox::warning(this,tr("QManga"),
                             tr("Unable to update preferred rendering."));
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
        bool ok;
        int s = am->data().toInt(&ok);
        if (ok && s>=0 && s<Z::maxOrdering)
            a = (Z::Ordering)s;
        else if (ok && s==Z::maxOrdering)
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

    if (QMessageBox::question(this,tr("Delete album"),
                              tr("Are you sure to delete album '%1' and all it's contents from database?").arg(s),
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
            if (fi.exists() && (fi.isDir() || fi.isFile())) {
                if (!dl.contains(fi.path()))
                    dl << fi.path();
            } else
                edl << fname;
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
            if (fi.exists() && (fi.isDir() || fi.isFile())) {
                if (!dl.contains(fi.filePath()))
                    dl << fi.filePath();
            }
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
    for (int i=0;i<sortActions.count();i++) {
        bool ok;
        int s = sortActions.at(i)->data().toInt(&ok);
        if (ok && s>=0 && s<Z::maxOrdering)
            sortActions[i]->setChecked(((Z::Ordering)s)==order);
        else if (ok && s==Z::maxOrdering)
            sortActions[i]->setChecked(reverseOrder);
    }

    if (updateGUI)
        albumClicked(ui->srcAlbums->currentItem());

    zg->defaultOrdering = order;
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
    return ui->srcList->viewMode();
}

void ZSearchTab::loadSearchItems(QSettings &settings)
{
    if (searchHistoryModel==NULL) return;

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
    if (searchHistoryModel==NULL) return;

    settings.setValue("search_history",QVariant::fromValue(searchHistoryModel->getHistoryItems()));
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
    emit dbGetFiles(item->text(),QString(),order,reverseOrder);
}

void ZSearchTab::mangaSearch()
{
    emit statusBarMsg(tr("Searching..."));

    ui->srcDesc->clear();

    if (model)
        model->deleteAllItems();

    QString s = ui->srcEdit->text();

    searchHistoryModel->appendHistoryItem(s);

    emit dbGetFiles(QString(),s,order,reverseOrder);
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
    QAction *ac = qobject_cast<QAction *>(sender());
    if (ac==NULL) return;
    bool delFiles = (ac->data().toInt() == 2);

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
    QFileInfoList fl = d.entryInfoList(QStringList("*"));
    QStringList files;
    for (int i=0;i<fl.count();i++)
        if (fl.at(i).isReadable() && fl.at(i).isFile() &&
                supportedImg().contains(fl.at(i).suffix(),Qt::CaseInsensitive))
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
        ui->srcList->setWordWrap(true);
        ui->srcList->setViewMode(QListView::IconMode);
        ui->srcList->setGridSize(gridSize(ui->srcIconSize->value()));
    } else {
        ui->srcList->setWordWrap(false);                // QTBUG-11227 (incorrect rect height in view
        ui->srcList->setViewMode(QListView::ListMode);  // item delegate when wordWrap enabled)
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
    emit statusBarMsg(QString("MBOX#Added %1 out of %2 found files in %3s").arg(count).arg(total).arg((double)elapsed/1000.0,1,'f',2));
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

void ZSearchTab::dbShowProgressDialog(const bool visible, const QString& title)
{
    if (visible) {
        if (progressDlg==NULL) {
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
    if (progressDlg!=NULL) {
        progressDlg->setValue(value);
        progressDlg->setLabelText(msg);
    }
}
