#ifndef ZSEARCHTAB_H
#define ZSEARCHTAB_H

#include <QWidget>
#include <QSlider>
#include <QRadioButton>
#include <QProgressDialog>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QSettings>

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
    enum LoadingState {
        lsLoading,
        lsLoaded
    };
    Q_ENUM(LoadingState)

    explicit ZSearchTab(QWidget *parent = nullptr);
    ~ZSearchTab() override;

    void updateAlbumsList();
    void updateFocus();
    QStringList getAlbums();
    QSize getPreferredCoverSize() const;
    void applySortOrder(Z::Ordering order, Qt::SortOrder direction);

private:
    Q_DISABLE_COPY(ZSearchTab)

    Ui::ZSearchTab *ui;
    Z::Ordering m_savedOrdering { Z::ordUndefined };
    Qt::SortOrder m_savedOrderingDirection { Qt::AscendingOrder };
    Z::Ordering m_defaultOrdering { Z::ordName };
    Qt::SortOrder m_defaultOrderingDirection { Qt::AscendingOrder };
    LoadingState m_loadingState { lsLoading };
    QString m_descTemplate;
    QStringList m_cachedAlbums;
    QPointer<ZMangaModel> m_model;
    QPointer<ZMangaSearchHistoryModel> m_searchHistoryModel;
    QPointer<ZMangaTableModel> m_tableModel;
    QPointer<ZMangaIconModel> m_iconModel;
    QPointer<QProgressDialog> progressDlg;

    QSize gridSize(int ref);
    QString getAlbumNameToAdd(const QString &suggest, int toAddCount);
    QFileInfoList getSelectedMangaEntries(bool includeDirs = true) const;
    QAbstractItemView* activeView() const;
    QModelIndexList getSelectedIndexes() const;
    QModelIndex mapToSource(const QModelIndex &index);
    int getIconSize() const;
    QListView::ViewMode getListViewMode() const;
    void setListViewOptions(QListView::ViewMode mode, int iconSize);
    void setDescText(const QString& text = QString());
    void updateLoadingState(ZSearchTab::LoadingState state);

public Q_SLOTS:
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
    void ctxEditMenu(const QPoint &pos);
    void ctxRenameAlbum();
    void ctxDeleteAlbum();
    void ctxAddEmptyAlbum();
    void ctxMoveAlbumToTopLevel();
    void ctxOpenDir();
    void ctxXdgOpen();
    void ctxFileCopy();
    void ctxFileCopyClipboard();
    void ctxChangeRenderer();

    void dbShowProgressDialog(bool visible);
    void dbShowProgressDialogEx(bool visible, const QString &title);
    void dbShowProgressState(int value, const QString& msg);
    void dbAlbumsListUpdated();
    void dbAlbumsListReady(const ZAlbumVector &albums);
    void dbFilesAdded(int count, int total, qint64 elapsed);
    void dbFilesLoaded(int count, qint64 elapsed);
    void dbErrorMsg(const QString& msg);
    void dbNeedTableCreation();

    void loadSettings(QSettings *settings);
    void saveSettings(QSettings *settings);

Q_SIGNALS:
    void mangaDblClick(const QString &filename);
    void statusBarMsg(const QString &msg);
    void dbRenameAlbum(const QString& oldName, const QString& newName);
    void dbGetAlbums();
    void dbCreateTables();
    void dbAddFiles(const QStringList& aFiles, const QString& album);
    void dbGetFiles(const QString& album, const QString& search, const QSize& preferredCoverSize);
    void dbDelFiles(const ZIntVector& dbids, bool fullDelete);
    void dbDeleteAlbum(const QString& album);
    void dbAddAlbum(const QString& album, const QString& parent);
    void dbReparentAlbum(const QString& album, const QString& parent);
    void dbSetPreferredRendering(const QString& filename, int mode);

};

#endif // ZSEARCHTAB_H
