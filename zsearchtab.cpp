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
}

ZSearchTab::~ZSearchTab()
{
    delete ui;
}

void ZSearchTab::updateAlbumsList()
{
    ui->srcAlbums->clear();
    ui->srcAlbums->addItems(zGlobal->sqlGetAlbums());
    mangaList.clear();
    updateModel(NULL);
}

void ZSearchTab::updateModel(SQLMangaList *list)
{
    QItemSelectionModel *m = ui->srcList->selectionModel();
    QAbstractItemModel *n = ui->srcList->model();
    ui->srcList->setModel(new ZMangaModel(this,list,ui->srcIconSize,ui->srcList));
    m->deleteLater();
    n->deleteLater();

    ui->srcDesc->clear();
}

QSize ZSearchTab::gridSize(int ref)
{
    QFontMetrics fm(font());
    if (ui->srcList->viewMode()==QListView::IconMode)
        return QSize(ref,5*ref/4+fm.height()*3);

    return QSize(ui->srcList->width()/3,25*fm.height()/10);
}

void ZSearchTab::albumChanged(QListWidgetItem *current, QListWidgetItem *)
{
    if (current==NULL) return;

    mangaList = zGlobal->sqlGetFiles(current->text(),ZGlobal::smName,false);

    updateModel(&mangaList);
}

void ZSearchTab::albumClicked(QListWidgetItem *item)
{
    albumChanged(item,NULL);
}

void ZSearchTab::mangaClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    if (index.row()>=mangaList.length()) return;
    ui->srcDesc->clear();
    const SQLMangaEntry m = mangaList.at(index.row());

    QString msg = QString(descTemplate).
            arg(m.name).arg(m.pagesCount).arg(formatSize(m.fileSize)).arg(m.album).arg(m.fileMagic).
            arg(m.fileDT.toString("yyyy-MM-dd")).arg(m.addingDT.toString("yyyy-MM-dd"));

    ui->srcDesc->setHtml(msg);
}

void ZSearchTab::mangaOpen(const QModelIndex &index)
{
    if (!index.isValid()) return;
    if (index.row()>=mangaList.length()) return;
    emit mangaDblClick(mangaList.at(index.row()).filename);

    for(int i=0;i<mangaList.count();i++)
        qDebug() << i << mangaList.at(i).name;
}

void ZSearchTab::mangaAdd()
{
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
    QIntList dl;
    QStringList albums = zGlobal->sqlGetAlbums();
    QModelIndexList li = ui->srcList->selectionModel()->selectedIndexes();
    for (int i=0;i<li.count();i++) {
        if (li.at(i).row()>=0 && li.at(i).row()<mangaList.count())
            dl << mangaList.at(li.at(i).row()).dbid;
    }
    zGlobal->sqlDelFiles(dl);
    if (ui->srcAlbums->count()>0) {
        ui->srcAlbums->setCurrentRow(0);
        updateModel(&mangaList);
    } else
        updateModel(NULL);
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
