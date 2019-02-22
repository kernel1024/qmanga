#ifndef ZSEARCHTAB_H
#define ZSEARCHTAB_H

#include <QWidget>
#include <QSlider>
#include <QRadioButton>
#include <QProgressDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSettings>
#include <QStateMachine>

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
    void applySortOrder(Z::Ordering order);
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

    QSize gridSize(int ref);
    QString getAlbumNameToAdd(const QString &suggest, int toAddCount);
    QFileInfoList getSelectedMangaEntries(bool includeDirs = true) const;
    QAbstractItemView* activeView() const;
    QModelIndexList getSelectedIndexes() const;
    QModelIndex mapToSource(const QModelIndex &index);

public slots:
    void albumClicked(QListWidgetItem * item);

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
    void ctxOpenDir();
    void ctxXdgOpen();
    void ctxFileCopy();
    void ctxFileCopyClipboard();

    void dbShowProgressDialog(const bool visible);
    void dbShowProgressDialogEx(const bool visible, const QString &title);
    void dbShowProgressState(const int value, const QString& msg);
    void dbAlbumsListUpdated();
    void dbAlbumsListReady(const QStringList& albums);
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
    void dbGetFiles(const QString& album, const QString& search, const Z::Ordering order,
                    const bool reverseOrder);
    void dbDelFiles(const QIntList& dbids, const bool fullDelete);
    void dbDeleteAlbum(const QString& album);
};

#endif // ZSEARCHTAB_H
