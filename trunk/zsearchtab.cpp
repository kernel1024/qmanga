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

    ui->srcIconSize->setMinimum(16);
    ui->srcIconSize->setMaximum(maxPreviewSize);
    ui->srcIconSize->setValue(128);
    ui->srcList->setGridSize(gridSize(ui->srcIconSize->value()));
    ui->srcModeIcon->setChecked(ui->srcList->viewMode()==QListView::IconMode);
    ui->srcModeList->setChecked(ui->srcList->viewMode()==QListView::ListMode);

    connect(ui->srcAlbums,SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this,SLOT(albumChanged(QListWidgetItem*,QListWidgetItem*)));
    connect(ui->srcAlbums,SIGNAL(itemActivated(QListWidgetItem*)),this,SLOT(albumClicked(QListWidgetItem*)));
    connect(ui->srcList,SIGNAL(clicked(QModelIndex)),this,SLOT(mangaClicked(QModelIndex)));
    connect(ui->srcList,SIGNAL(activated(QModelIndex)),this,SLOT(mangaOpen(QModelIndex)));
    connect(ui->srcAddBtn,SIGNAL(clicked()),this,SLOT(mangaAdd()));
    connect(ui->srcAddDirBtn,SIGNAL(clicked()),this,SLOT(mangaAddDir()));
    connect(ui->srcDelBtn,SIGNAL(clicked()),this,SLOT(mangaDel()));
    connect(ui->srcModeIcon,SIGNAL(clicked()),this,SLOT(listModeChanged()));
    connect(ui->srcModeList,SIGNAL(clicked()),this,SLOT(listModeChanged()));
    connect(ui->srcIconSize,SIGNAL(valueChanged(int)),this,SLOT(iconSizeChanged(int)));

    connect(&loader,SIGNAL(finished()),this,SLOT(loaderFinished()));

    model = new ZMangaModel(this,&mangaList,ui->srcIconSize,ui->srcList,&listUpdating);
    ui->srcList->setModel(model);
}

ZSearchTab::~ZSearchTab()
{
    delete ui;
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
}

QSize ZSearchTab::gridSize(int ref)
{
    QFontMetrics fm(font());
    if (ui->srcList->viewMode()==QListView::IconMode)
        return QSize(ref+25,ref*previewProps+fm.height()*3);

    return QSize(ui->srcList->width()/3,25*fm.height()/10);
}

void ZSearchTab::albumChanged(QListWidgetItem *current, QListWidgetItem *)
{
    if (current==NULL) return;
    if (loadingNow) return;

    ui->srcDesc->clear();

    loadingNow = true;
    updateWidgetsState();
    loader.setParams(&mangaList,&listUpdating,model,current->text(),QString());
    loader.start();
}

void ZSearchTab::albumClicked(QListWidgetItem *item)
{
    albumChanged(item,NULL);
}

void ZSearchTab::mangaClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    listUpdating.lock();
    int maxl = mangaList.length();
    listUpdating.unlock();

    if (index.row()>=maxl) return;

    ui->srcDesc->clear();

    int idx = index.row();
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
    QFileInfo fi(fl.first());
    zGlobal->savedIndexOpenDir = fi.path();

    QDir d(fi.path());
    QString album = d.dirName();
    bool ok;
    album = QInputDialog::getText(this,tr("QManga"),tr("Album name"),QLineEdit::Normal,album,&ok);
    if (ok)
        zGlobal->sqlAddFiles(fl,album);

    updateAlbumsList();
}

void ZSearchTab::mangaAddDir()
{
    if (loadingNow) return;
    QString fi = zGlobal->getExistingDirectoryD(this,tr("Add manga to index from directory"),
                                                    zGlobal->savedIndexOpenDir);
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
    if (ok)
        zGlobal->sqlAddFiles(files,album);

    updateAlbumsList();
}

void ZSearchTab::mangaDel()
{
    if (loadingNow) return;
    QIntList dl;
    QStringList albums = zGlobal->sqlGetAlbums();
    QModelIndexList li = ui->srcList->selectionModel()->selectedIndexes();

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

void ZSearchTab::listModeChanged()
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
    updateWidgetsState();
}
