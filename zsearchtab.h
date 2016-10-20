#ifndef ZSEARCHTAB_H
#define ZSEARCHTAB_H

#include <QWidget>
#include <QSlider>
#include <QRadioButton>
#include <QProgressDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSettings>

#include "zglobal.h"
#include "zmangamodel.h"

namespace Ui {
class ZSearchTab;
}

class ZSearchTab : public QWidget
{
    Q_OBJECT
    
public:
    explicit ZSearchTab(QWidget *parent = 0);
    ~ZSearchTab();

    void updateAlbumsList();
    void applyOrder(Z::Ordering aOrder, bool aReverseOrder, bool updateGUI = false);
    QStringList getAlbums();
    void setListViewOptions(const QListView::ViewMode mode, const int iconSize);
    int getIconSize() const;
    QListView::ViewMode getListViewMode() const;

    void loadSearchItems(QSettings &settings);
    void saveSearchItems(QSettings &settings);
private:
    Ui::ZSearchTab *ui;
    QStateMachine loadingState;
    QString descTemplate;
    QPointer<ZMangaModel> model;
    QProgressDialog *progressDlg;
    QStringList cachedAlbums;
    Z::Ordering order;
    bool reverseOrder;
    QList<QPointer<QAction> > sortActions;
    ZMangaSearchHistoryModel* searchHistoryModel;

    QSize gridSize(int ref);
    QString getAlbumNameToAdd(QString suggest,int toAddCount);

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
    void updateSplitters();

    void headerClicked(int logicalIndex);

    void ctxMenu(QPoint pos);
    void ctxAlbumMenu(QPoint pos);
    void ctxSorting();
    void ctxRenameAlbum();
    void ctxDeleteAlbum();
    void ctxOpenDir();
    void ctxXdgOpen();

    void dbShowProgressDialog(const bool visible, const QString &title = QString());
    void dbShowProgressState(const int value, const QString& msg);
    void dbAlbumsListUpdated();
    void dbAlbumsListReady(const QStringList& albums);
    void dbFilesAdded(const int count, const int total, const int elapsed);
    void dbFilesLoaded(const int count, const int elapsed);
    void dbErrorMsg(const QString& msg);
    void dbNeedTableCreation();
    void ctxChangeRenderer();

signals:
    void mangaDblClick(QString filename);
    void statusBarMsg(QString msg);
    void dbRenameAlbum(const QString& oldName, const QString& newName);
    void dbGetAlbums();
    void dbCreateTables();
    void dbAddFiles(const QStringList& aFiles, const QString& album);
    void dbGetFiles(const QString& album, const QString& search, const Z::Ordering order, const bool reverseOrder);
    void dbDelFiles(const QIntList& dbids, const bool fullDelete);
    void dbDeleteAlbum(const QString& album);
};

#endif // ZSEARCHTAB_H
