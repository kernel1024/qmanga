#ifndef ZSEARCHTAB_H
#define ZSEARCHTAB_H

#include <QWidget>
#include <QSlider>
#include <QRadioButton>
#include <QProgressDialog>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QSettings>
#include <QStateMachine>

#include "global.h"
#include "zglobal.h"
#include "zmangamodel.h"

namespace Ui {
class ZSearchTab;
}

class ZSearchTab : public QWidget
{
    Q_OBJECT
    
public:
    explicit ZSearchTab(QWidget *parent = nullptr);
    ~ZSearchTab();

    void updateAlbumsList();
    void updateFocus();
    QStringList getAlbums();
    void setListViewOptions(const QListView::ViewMode mode, int iconSize);
    int getIconSize() const;
    QListView::ViewMode getListViewMode() const;

    void loadSearchItems(QSettings &settings);
    void saveSearchItems(QSettings &settings);
    void applySortOrder(Z::Ordering order, Qt::SortOrder direction);
private:
    Ui::ZSearchTab *ui;
    QStateMachine loadingState;
    QString descTemplate;
    QPointer<ZMangaModel> model;
    QProgressDialog *progressDlg;
    QStringList cachedAlbums;
    ZMangaSearchHistoryModel* searchHistoryModel;
    ZMangaTableModel* tableModel;
    ZMangaIconModel* iconModel;
    Z::Ordering savedOrdering;
    Qt::SortOrder savedOrderingDirection;

    QSize gridSize(int ref);
    QString getAlbumNameToAdd(const QString &suggest, int toAddCount);
    QFileInfoList getSelectedMangaEntries(bool includeDirs = true) const;
    QAbstractItemView* activeView() const;
    QModelIndexList getSelectedIndexes() const;
    QModelIndex mapToSource(const QModelIndex &index);
    void setDescText(const QString& text = QString());

public slots:
    void albumClicked(QTreeWidgetItem *item, int column);

    void mangaSearch();
    void mangaSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
    void mangaOpen(const QModelIndex &index);
    void mangaAdd();
    void mangaAddDir();
    void mangaDel();
    void imagesAddDir();

    void listModeChanged(bool state);
    void iconSizeChanged(int ref);
    void sortingChanged(int logicalIndex, Qt::SortOrder order);
    void updateSplitters();

    void ctxMenu(const QPoint &pos);
    void ctxAlbumMenu(const QPoint &pos);
    void ctxRenameAlbum();
    void ctxDeleteAlbum();
    void ctxAddEmptyAlbum();
    void ctxMoveAlbumToTopLevel();
    void ctxOpenDir();
    void ctxXdgOpen();
    void ctxFileCopy();
    void ctxFileCopyClipboard();

    void dbShowProgressDialog(const bool visible);
    void dbShowProgressDialogEx(const bool visible, const QString &title);
    void dbShowProgressState(const int value, const QString& msg);
    void dbAlbumsListUpdated();
    void dbAlbumsListReady(const AlbumVector &albums);
    void dbFilesAdded(const int count, const int total, const int elapsed);
    void dbFilesLoaded(const int count, const int elapsed);
    void dbErrorMsg(const QString& msg);
    void dbNeedTableCreation();
    void ctxChangeRenderer();

signals:
    void mangaDblClick(const QString &filename);
    void statusBarMsg(const QString &msg);
    void dbRenameAlbum(const QString& oldName, const QString& newName);
    void dbGetAlbums();
    void dbCreateTables();
    void dbAddFiles(const QStringList& aFiles, const QString& album);
    void dbGetFiles(const QString& album, const QString& search);
    void dbDelFiles(const QIntVector& dbids, const bool fullDelete);
    void dbDeleteAlbum(const QString& album);
    void dbAddAlbum(const QString& album, const QString& parent);
    void dbReparentAlbum(const QString& album, const QString& parent);
    bool dbSetPreferredRendering(const QString& filename, int mode);

};

#endif // ZSEARCHTAB_H
