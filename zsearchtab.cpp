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

namespace ZDefaults {
constexpr int stackIcons = 0;
constexpr int stackTable = 1;
}

ZSearchTab::ZSearchTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ZSearchTab)
{
    ui->setupUi(this);

    m_descTemplate = ui->srcDesc->text();
    setDescText();
    ui->srcLoading->hide();

    auto completer = new QCompleter(this);
    m_searchHistoryModel = new ZMangaSearchHistoryModel(completer);
    completer->setModel(m_searchHistoryModel);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    ui->srcEdit->setCompleter(completer);
    ui->srcEdit->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->srcIconSize->setMinimum(ZDefaults::minPreviewSize);
    ui->srcIconSize->setMaximum(ZDefaults::maxPreviewSize);
    ui->srcIconSize->setValue(ZDefaults::previewSize);
    ui->srcList->setGridSize(gridSize(ui->srcIconSize->value()));
    ui->srcList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->srcModeIcon->setChecked(ui->srcStack->currentIndex()==ZDefaults::stackIcons);
    ui->srcModeList->setChecked(ui->srcStack->currentIndex()==ZDefaults::stackTable);
    ui->srcAlbums->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->srcAlbums,&QTreeWidget::itemClicked,this,&ZSearchTab::albumClicked);
    connect(ui->srcAlbums,&QTreeWidget::customContextMenuRequested,
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
    connect(ui->srcEdit,&QLineEdit::customContextMenuRequested,
            this,&ZSearchTab::ctxEditMenu);
    connect(ui->srcEditBtn,&QPushButton::clicked,this,&ZSearchTab::mangaSearch);

    connect(zg->db,&ZDB::albumsListUpdated,this,&ZSearchTab::dbAlbumsListUpdated,Qt::QueuedConnection);
    connect(zg->db,&ZDB::gotAlbums,this,&ZSearchTab::dbAlbumsListReady,Qt::QueuedConnection);
    connect(zg->db,&ZDB::filesAdded,this,&ZSearchTab::dbFilesAdded,Qt::QueuedConnection);
    connect(zg->db,&ZDB::filesLoaded,this,&ZSearchTab::dbFilesLoaded,Qt::QueuedConnection);
    connect(zg->db,&ZDB::errorMsg,this,&ZSearchTab::dbErrorMsg,Qt::QueuedConnection);
    connect(zg->db,&ZDB::needTableCreation,this,&ZSearchTab::dbNeedTableCreation,Qt::QueuedConnection);
    connect(zg->db,&ZDB::showProgressDialog,
            this,&ZSearchTab::dbShowProgressDialog,Qt::QueuedConnection);
    connect(zg->db,&ZDB::showProgressState,this,&ZSearchTab::dbShowProgressState,Qt::QueuedConnection);

    connect(this,&ZSearchTab::dbAddFiles,zg->db,&ZDB::sqlAddFiles,Qt::QueuedConnection);
    connect(this,&ZSearchTab::dbDelFiles,zg->db,&ZDB::sqlDelFiles,Qt::QueuedConnection);
    connect(this,&ZSearchTab::dbGetAlbums,zg->db,&ZDB::sqlGetAlbums,Qt::QueuedConnection);
    connect(this,&ZSearchTab::dbGetFiles,zg->db,&ZDB::sqlGetFiles,Qt::QueuedConnection);
    connect(this,&ZSearchTab::dbRenameAlbum,zg->db,&ZDB::sqlRenameAlbum,Qt::QueuedConnection);
    connect(this,&ZSearchTab::dbCreateTables,zg->db,&ZDB::sqlCreateTables,Qt::QueuedConnection);
    connect(this,&ZSearchTab::dbDeleteAlbum,zg->db,&ZDB::sqlDelAlbum,Qt::QueuedConnection);
    connect(this,&ZSearchTab::dbAddAlbum,zg->db,&ZDB::sqlAddAlbum,Qt::QueuedConnection);
    connect(this,&ZSearchTab::dbReparentAlbum,zg->db,&ZDB::sqlReparentAlbum,Qt::QueuedConnection);
    connect(this,&ZSearchTab::dbSetPreferredRendering,zg->db,&ZDB::sqlSetPreferredRendering,
            Qt::QueuedConnection);

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
    m_loadingState.addState(sLoading);
    m_loadingState.addState(sLoaded);
    m_loadingState.setInitialState(sLoaded);
    m_loadingState.start();

    auto aModel = new ZMangaModel(this,ui->srcIconSize,ui->srcTable);
    m_tableModel = new ZMangaTableModel(this,ui->srcTable);
    m_iconModel = new ZMangaIconModel(this,ui->srcList);
    m_tableModel->setSourceModel(aModel);
    m_iconModel->setSourceModel(aModel);
    ui->srcList->setModel(m_iconModel);
    ui->srcTable->setModel(m_tableModel);
    connect(ui->srcTable->horizontalHeader(),&QHeaderView::sortIndicatorChanged,
            this,&ZSearchTab::sortingChanged);

    connect(ui->srcList->selectionModel(),&QItemSelectionModel::currentChanged,
            this,&ZSearchTab::mangaSelectionChanged);
    connect(ui->srcTable->selectionModel(),&QItemSelectionModel::currentChanged,
            this,&ZSearchTab::mangaSelectionChanged);

    connect(zg->db,&ZDB::gotFile,aModel,&ZMangaModel::addItem,Qt::QueuedConnection);
    connect(zg->db,&ZDB::deleteItemsFromModel,aModel,&ZMangaModel::deleteItems,Qt::QueuedConnection);
    m_model = aModel;

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
    widths << ZDefaults::albumListWidth;
    widths << width()-widths[0];
    ui->splitter->setSizes(widths);
}

void ZSearchTab::ctxMenu(const QPoint &pos)
{
    QAbstractItemView *view = activeView();
    QAction *acm;
    QMenu cm(view);

    if (m_savedOrdering == Z::UndefinedOrder) { // Disable sorting control for dynamic albums
        QMenu* smenu = cm.addMenu(QIcon(":/16/view-sort-ascending"),tr("Sort"));

        for (int i=0;i<Z::maxOrdering;i++) {
            acm = new QAction(Z::sortMenu.value(static_cast<Z::Ordering>(i)),this);
            acm->setCheckable(true);
            acm->setChecked(static_cast<int>(m_defaultOrdering) == i);
            connect(acm,&QAction::triggered,this,[this,i](){
                applySortOrder(static_cast<Z::Ordering>(i),m_defaultOrderingDirection);
            });
            smenu->addAction(acm);
        }
        smenu->addSeparator();

        acm = new QAction(tr("Reverse order"),this);
        acm->setCheckable(true);
        acm->setChecked(m_defaultOrderingDirection == Qt::DescendingOrder);
        connect(acm,&QAction::triggered,this,[this](){
            Qt::SortOrder order = Qt::AscendingOrder;
            if (m_defaultOrderingDirection == Qt::AscendingOrder)
                order = Qt::DescendingOrder; // reverse
            applySortOrder(m_defaultOrdering,order);
        });
        smenu->addAction(acm);
        cm.addSeparator();
    }

    acm = cm.addAction(QIcon(QSL(":/16/edit-select-all")),tr("Select All"));
    connect(acm,&QAction::triggered,view,&QAbstractItemView::selectAll);

    cm.addSeparator();

    const QModelIndexList li = getSelectedIndexes();
    int cnt = 0;
    for (const auto &i : li) {
        if (i.column()==0)
            cnt++;
    }
    QMenu* dmenu = cm.addMenu(QIcon(QSL(":/16/edit-delete")),
                              tr("Delete selected %1 files").arg(cnt));
    acm = dmenu->addAction(tr("Delete only from database"),this,&ZSearchTab::mangaDel);
    acm->setEnabled(cnt>0 && !ui->srcLoading->isVisible());
    acm->setData(1);
    acm = dmenu->addAction(tr("Delete from database and filesystem"),this,&ZSearchTab::mangaDel);
    acm->setEnabled(cnt>0 && !ui->srcLoading->isVisible());
    acm->setData(2);

    acm = cm.addAction(QIcon(QSL(":/16/document-open-folder")),
                       tr("Open containing directory"),this,&ZSearchTab::ctxOpenDir);
    acm->setEnabled(cnt>0 && !ui->srcLoading->isVisible());

    cm.addAction(QIcon(QSL(":/16/fork")),
                 tr("Open with default DE action"),this,&ZSearchTab::ctxXdgOpen);

    cm.addSeparator();

#ifndef Q_OS_WIN
    cm.addAction(QIcon(QSL(":/16/edit-copy")),
                 tr("Copy files"),this,&ZSearchTab::ctxFileCopyClipboard);
#endif

    cm.addAction(QIcon(QSL(":/16/edit-copy")),
                 tr("Copy files to directory..."),this,&ZSearchTab::ctxFileCopy);

    if (li.count()==1) {
        SQLMangaEntry m = m_model->getItem(li.first().row());
        if (m.fileMagic == QSL("PDF")) {
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

    SQLMangaEntry m = m_model->getItem(li.first().row());

    bool ok;
    int mode = am->data().toInt(&ok);
    if (ok) {
        Q_EMIT dbSetPreferredRendering(m.filename,mode);
    } else {
        QMessageBox::warning(this,tr("QManga"),
                             tr("Unable to update preferred rendering."));
    }

    albumClicked(ui->srcAlbums->currentItem(),0);

}

void ZSearchTab::ctxAlbumMenu(const QPoint &pos)
{
    QTreeWidgetItem* itm = ui->srcAlbums->itemAt(pos);

    QMenu cm(ui->srcAlbums);
    QAction *acm;
    if (itm) {
        acm = cm.addAction(QIcon(QSL(":/16/edit-rename")),tr("Rename album"),
                           this,&ZSearchTab::ctxRenameAlbum);
        acm->setData(itm->text(0));
    }

    acm = cm.addAction(QIcon(QSL(":/16/document-new")),tr("Add empty album"),
                       this,&ZSearchTab::ctxAddEmptyAlbum);

    if (itm) {
        if (itm->parent()) {
            acm = cm.addAction(QIcon(QSL(":/16/go-previous-view-page")),
                               tr("Move to top level"),this,&ZSearchTab::ctxMoveAlbumToTopLevel);
            acm->setData(itm->text(0));
        }

        cm.addSeparator();

        acm = cm.addAction(QIcon(QSL(":/16/edit-delete")),tr("Delete album"),
                           this,&ZSearchTab::ctxDeleteAlbum);
        acm->setData(itm->text(0));
    }

    cm.exec(ui->srcAlbums->mapToGlobal(pos));
}

void ZSearchTab::ctxEditMenu(const QPoint &pos)
{
    QMenu* menu = ui->srcEdit->createStandardContextMenu();
    QAction* ac;

    menu->insertSeparator(menu->actions().first());

    ac = new QAction(QIcon::fromTheme("edit-clear"),tr("Clear"),menu);
    connect(ac,&QAction::triggered,this,[this](){
        ui->srcEdit->clear();
    });
    menu->insertAction(menu->actions().first(),ac);

    menu->exec(ui->srcEdit->mapToGlobal(pos));
    menu->deleteLater();
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

    Q_EMIT dbRenameAlbum(s,n);
}

void ZSearchTab::ctxDeleteAlbum()
{
    auto nt = qobject_cast<QAction *>(sender());
    if (nt==nullptr) return;
    QString s = nt->data().toString();
    if (s.isEmpty()) return;

    if (QMessageBox::question(this,tr("Delete album"),
                              tr("Are you sure to delete album '%1' and all it's contents from database?\n"
                                 "Sub-albums will be moved to top level.").arg(s),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No)==QMessageBox::Yes)
        Q_EMIT dbDeleteAlbum(s);
}

void ZSearchTab::ctxAddEmptyAlbum()
{
    QString name = QInputDialog::getText(this,tr("Add empty album"),tr("Album name"));
    if (!name.isEmpty())
        Q_EMIT dbAddAlbum(name,QString());
}

void ZSearchTab::ctxMoveAlbumToTopLevel()
{
    auto nt = qobject_cast<QAction *>(sender());
    if (nt==nullptr) return;
    const QString s = nt->data().toString();
    if (s.isEmpty()) return;

    Q_EMIT dbReparentAlbum(s,QString());
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
    data->setData(QSL("text/uri-list"),uriList);
    data->setData(QSL("text/plain"),plainList);
    data->setData(QSL("application/x-kde4-urilist"),uriList);

    QClipboard *cl = QApplication::clipboard();
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
    Q_EMIT dbGetAlbums();
}

void ZSearchTab::updateFocus()
{
    if (activeView()->model()->rowCount()==0)
        ui->srcEdit->setFocus();
}

QStringList ZSearchTab::getAlbums()
{
    return m_cachedAlbums;
}

void ZSearchTab::setListViewOptions(QListView::ViewMode mode, int iconSize)
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
    if (ui->srcStack->currentIndex()==ZDefaults::stackIcons)
        return QListView::IconMode;

    return QListView::ListMode;
}

void ZSearchTab::loadSettings(QSettings *settings)
{
    if (m_searchHistoryModel==nullptr) return;

    if (settings!=nullptr) {
        // TODO: backward compatibility. Delete this sometime after.
        settings->beginGroup(QSL("MainWindow"));
        QStringList history = settings->value(QSL("search_history"),QStringList()).toStringList();
        settings->endGroup();
        // -----------

        settings->beginGroup(QSL("SearchTab"));

        QListView::ViewMode mode = QListView::IconMode;
        if (settings->value(QSL("listMode"),false).toBool())
            mode = QListView::ListMode;
        int iconSize = settings->value(QSL("iconSize"),ZDefaults::previewSize).toInt();
        setListViewOptions(mode, iconSize);

        m_defaultOrdering = static_cast<Z::Ordering>(settings->value(QSL("defaultOrdering"),
                                                                     Z::Name).toInt());
        m_defaultOrderingDirection = static_cast<Qt::SortOrder>(settings->value(
                                                                    QSL("defaultOrderingDirection"),
                                                                    Qt::AscendingOrder).toInt());

        if (history.isEmpty())
            history = settings->value(QSL("search_history"),QStringList()).toStringList();

        settings->endGroup();

        m_searchHistoryModel->setHistoryItems(history);
    }

    bool isDBaccessible = zg->dbEngine!=Z::UndefinedDB;
    setEnabled(isDBaccessible);
    if (isDBaccessible) {
        updateAlbumsList();
        if (settings!=nullptr)
            applySortOrder(m_defaultOrdering,m_defaultOrderingDirection);
    }
}

void ZSearchTab::saveSettings(QSettings *settings)
{
    if (m_searchHistoryModel==nullptr) return;

    settings->beginGroup(QSL("SearchTab"));

    settings->setValue(QSL("listMode"),getListViewMode()==QListView::ListMode);
    settings->setValue(QSL("iconSize"),getIconSize());
    settings->setValue(QSL("defaultOrdering"),static_cast<int>(m_defaultOrdering));
    settings->setValue(QSL("defaultOrderingDirection"),
                      static_cast<int>(m_defaultOrderingDirection));
    settings->setValue(QSL("search_history"),
                       QVariant::fromValue(m_searchHistoryModel->getHistoryItems()));

    settings->endGroup();
}

void ZSearchTab::applySortOrder(Z::Ordering order, Qt::SortOrder direction)
{
    int col = static_cast<int>(order);
    if (col>=0 && col<ui->srcTable->model()->columnCount())
        ui->srcTable->sortByColumn(col,direction);
}

void ZSearchTab::sortingChanged(int logicalIndex, Qt::SortOrder order)
{
    m_iconModel->sort(logicalIndex,order);
    if (logicalIndex>=0 && logicalIndex<Z::maxOrdering) {
        m_defaultOrdering = static_cast<Z::Ordering>(logicalIndex);
        m_defaultOrderingDirection = order;
    }
}

QSize ZSearchTab::gridSize(int ref)
{
    QFontMetrics fm(font());
    return QSize(ref+ZDefaults::previewWidthMargin,
                 qRound(ref*ZDefaults::previewProps+fm.height()*4));
}

QString ZSearchTab::getAlbumNameToAdd(const QString &suggest, int toAddCount)
{
    auto dlg = new ZAlbumSelectorDlg(this,m_cachedAlbums,suggest,toAddCount);
    QString ret = QString();
    if (dlg->exec()==QDialog::Accepted) {
        ret = dlg->getAlbumName();
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
        if (i.row()>=0 && i.row()<m_model->getItemsCount()) {
            QString fname = m_model->getItem(i.row()).filename;
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
    if (ui->srcStack->currentIndex()==ZDefaults::stackTable)
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

void ZSearchTab::setDescText(const QString &text)
{
    ui->srcDesc->setVisible(!text.isEmpty());
    ui->srcDesc->setText(text);
}

void ZSearchTab::albumClicked(QTreeWidgetItem *item, int column)
{
    if (item==nullptr) return;

    Q_EMIT statusBarMsg(tr("Searching..."));

    setDescText();

    if (m_model)
        m_model->deleteAllItems();

    const QString album = item->text(0);

    if (album.startsWith(QSL("# "))) {
        if (m_savedOrdering==Z::UndefinedOrder) {
            m_savedOrdering = m_defaultOrdering;
            m_savedOrderingDirection = m_defaultOrderingDirection;
        }
        Qt::SortOrder dir;
        Z::Ordering ord = zg->db->getDynAlbumOrdering(album,dir);
        if (ord!=Z::UndefinedOrder)
            applySortOrder(ord,dir);

    } else if (!album.startsWith(QSL("% ")) &&
               (m_savedOrdering!=Z::UndefinedOrder)) {
        applySortOrder(m_savedOrdering,m_savedOrderingDirection);
        m_savedOrdering = Z::UndefinedOrder;
    }

    Q_UNUSED(column)
    Q_EMIT dbGetFiles(album,QString());
}

void ZSearchTab::mangaSearch()
{
    Q_EMIT statusBarMsg(tr("Searching..."));

    setDescText();

    if (m_model)
        m_model->deleteAllItems();

    QString s = ui->srcEdit->text().trimmed();

    m_searchHistoryModel->appendHistoryItem(s);

    Q_EMIT dbGetFiles(QString(),s);
}

void ZSearchTab::mangaSelectionChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous)

    setDescText();
    if (!current.isValid()) return;
    if (!m_model) return;

    int maxl = m_model->getItemsCount();
    int row = mapToSource(current).row();

    if (row>=maxl) return;

    setDescText();

    int idx = row;
    const SQLMangaEntry m = m_model->getItem(idx);

    QFileInfo fi(m.filename);

    QString msg = QString(m_descTemplate)
                  .arg(elideString(m.name,ZDefaults::maxDescriptionStringLength,Qt::ElideMiddle))
                  .arg(m.pagesCount)
                  .arg(formatSize(m.fileSize),m.album,m.fileMagic,
                       m.fileDT.toString(QSL("yyyy-MM-dd")),
                       m.addingDT.toString(QSL("yyyy-MM-dd")),
                       elideString(fi.path(),ZDefaults::maxDescriptionStringLength,Qt::ElideMiddle));

    setDescText(msg);
}

void ZSearchTab::mangaOpen(const QModelIndex &index)
{
    if (!index.isValid()) return;
    if (!m_model) return;

    int maxl = m_model->getItemsCount();

    int idx = mapToSource(index).row();
    if (idx>=maxl) return;

    QString filename = m_model->getItem(idx).filename;

    if (m_model->getItem(idx).fileMagic==QSL("DYN"))
        filename.prepend(QSL("#DYN#"));

    Q_EMIT mangaDblClick(filename);
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

    Q_EMIT dbAddFiles(fl,album);
}

void ZSearchTab::mangaAddDir()
{
    QString fi = getExistingDirectoryD(this,tr("Add manga to index from directory"),
                                                    zg->savedIndexOpenDir);
    if (fi.isEmpty()) return;
    zg->savedIndexOpenDir = fi;

    QDir d(fi);
    d.setFilter(QDir::Files | QDir::Readable);
    d.setNameFilters(QStringList(QSL("*")));
    QStringList files;
    QDirIterator it(d, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
    while (it.hasNext())
        files << it.next();

    QString album = d.dirName();

    album = getAlbumNameToAdd(album,-1);
    if (album.isEmpty()) return;

    Q_EMIT dbAddFiles(files,album);
}

void ZSearchTab::mangaDel()
{
    if (!m_model) return;
    auto ac = qobject_cast<QAction *>(sender());
    if (ac==nullptr) return;
    bool delFiles = (ac->data().toInt() == 2);

    QIntVector dl;
    int cnt = zg->db->getAlbumsCount();
    // remove other selected columns
    const QModelIndexList lii = getSelectedIndexes();
    QModelIndexList li;
    for (const auto &i : lii) {
        if (i.column()==0)
            li << i;
    }

    setDescText();

    for (const auto &i : qAsConst(li)) {
        if (i.row()>=0 && i.row()<m_model->getItemsCount())
            dl << m_model->getItem(i.row()).dbid;
    }

    Q_EMIT dbDelFiles(dl,delFiles);

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
    const QFileInfoList fl = filterSupportedImgFiles(
                                 d.entryInfoList(QStringList(QSL("*")),
                                                 QDir::Files | QDir::Readable));
    QStringList files;
    files.reserve(fl.count());
    for (const auto &i : fl)
        files << i.absoluteFilePath();

    if (files.isEmpty()) {
        QMessageBox::warning(this,tr("QManga"),
                             tr("Supported image files not found in specified directory."));
        return;
    }
    QString album = getAlbumNameToAdd(QString(),-1);
    if (album.isEmpty()) return;

    files.clear();
    files << QSL("###DYNAMIC###");
    files << fi;

    Q_EMIT dbAddFiles(files,album);
}

void ZSearchTab::listModeChanged(bool state)
{
    Q_UNUSED(state)

    if (ui->srcModeIcon->isChecked()) {
        ui->srcStack->setCurrentIndex(ZDefaults::stackIcons);
        ui->srcList->setGridSize(gridSize(ui->srcIconSize->value()));
    } else {
        ui->srcStack->setCurrentIndex(ZDefaults::stackTable);
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

void ZSearchTab::dbAlbumsListReady(const AlbumVector &albums)
{
    m_cachedAlbums.clear();
    ui->srcAlbums->clear();

    QTreeWidgetItem* dynRoot = nullptr;

    QHash<int,QTreeWidgetItem*> items;
    for (const auto& album : albums) {
        auto item = new QTreeWidgetItem();
        item->setText(0,album.name);
        item->setData(0,Qt::UserRole,album.id);
        items[album.id] = item;
        if (album.id>=0)
            m_cachedAlbums.append(album.name);
    }
    for (const auto& album : albums) {
        if (!items.contains(album.id)) continue;

        if (album.parent>=0 && items.contains(album.parent)) {
            items[album.parent]->addChild(items.value(album.id));

        } else if (album.parent == dynamicAlbumParent) {
            if (!dynRoot) {
                dynRoot = new QTreeWidgetItem();
                dynRoot->setText(0,tr("Dynamic albums"));
                dynRoot->setData(0,Qt::UserRole,dynamicAlbumParent);
            }
            dynRoot->addChild(items.value(album.id));

        } else {
            ui->srcAlbums->addTopLevelItem(items.value(album.id));
        }
    }
    if (dynRoot)
        ui->srcAlbums->addTopLevelItem(dynRoot);

    if (m_model)
        m_model->deleteAllItems();
}

void ZSearchTab::dbFilesAdded(int count, int total, int elapsed)
{
    Q_EMIT statusBarMsg(QSL("MBOX#Added %1 out of %2 found files in %3s")
                      .arg(count).arg(total).arg(static_cast<double>(elapsed)/ZDefaults::oneSecondMSf,1,'f',2));
}

void ZSearchTab::dbFilesLoaded(int count, int elapsed)
{
    Q_EMIT statusBarMsg(QSL("Found %1 results in %2s")
                      .arg(count).arg(static_cast<double>(elapsed)/ZDefaults::oneSecondMSf,1,'f',2));
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
        Q_EMIT dbCreateTables();
}

void ZSearchTab::dbShowProgressDialog(bool visible)
{
    dbShowProgressDialogEx(visible, QString());
}

void ZSearchTab::dbShowProgressDialogEx(bool visible, const QString& title)
{
    if (visible) {
        if (progressDlg==nullptr) {
            progressDlg = new QProgressDialog(tr("Adding files"),tr("Cancel"),0,100,this);
            connect(progressDlg,&QProgressDialog::canceled,
                    zg->db,&ZDB::sqlCancelAdding,Qt::QueuedConnection);
        }
        progressDlg->setWindowModality(Qt::WindowModal);
        if (title.isEmpty()) {
            progressDlg->setWindowTitle(tr("Adding files to index..."));
        } else {
            progressDlg->setWindowTitle(title);
        }
        progressDlg->setValue(0);
        progressDlg->setLabelText(QString());
        progressDlg->show();
    } else {
        progressDlg->hide();
    }
}

void ZSearchTab::dbShowProgressState(int value, const QString &msg)
{
    if (progressDlg!=nullptr) {
        progressDlg->setValue(value);
        progressDlg->setLabelText(msg);
    }
}
