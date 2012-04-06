#ifndef ZSEARCHTAB_H
#define ZSEARCHTAB_H

#include <QtCore>
#include <QtGui>
#include "zglobal.h"
#include "zmangamodel.h"

namespace Ui {
class ZSearchTab;
}

class ZSearchTab : public QWidget
{
    Q_OBJECT
    
public:
    QSlider* srclIconSize;
    QRadioButton* srclModeIcon;
    QRadioButton* srclModeList;
    Z::Ordering order;
    bool reverseOrder;

    explicit ZSearchTab(QWidget *parent = 0);
    ~ZSearchTab();

    void updateAlbumsList();
    void updateWidgetsState();
    
private:
    Ui::ZSearchTab *ui;
    bool loadingNow;
    QString descTemplate;
    ZMangaModel* model;
    QProgressDialog progressDlg;
    QStringList cachedAlbums;

    QSize gridSize(int ref);
    QString getAlbumNameToAdd(QString suggest);

public slots:
    void albumChanged(QListWidgetItem * current, QListWidgetItem * previous);
    void albumClicked(QListWidgetItem * item);
    void mangaSearch();
    void mangaSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
    void mangaOpen(const QModelIndex &index);
    void mangaAdd();
    void mangaAddDir();
    void mangaDel();
    void listModeChanged(bool state);
    void iconSizeChanged(int ref);
    void updateSplitters();
    void ctxMenu(QPoint pos);
    void albumCtxMenu(QPoint pos);

    void ctxSortName();
    void ctxSortAlbum();
    void ctxSortPage();
    void ctxSortAdded();
    void ctxSortCreated();
    void ctxReverseOrder();
    void ctxRenameAlbum();

    void showProgressDialog(const bool visible);
    void showProgressState(const int value, const QString& msg);
    void dbAlbumsListUpdated();
    void dbAlbumsListReady(QStringList& albums);
    void dbFilesAdded(const int count, const int total, const int elapsed);
    void dbFilesLoaded(const int count, const int elapsed);
    void dbErrorMsg(const QString& msg);
    void dbNeedTableCreation();

signals:
    void mangaDblClick(QString filename);
    void statusBarMsg(QString msg);
    void dbRenameAlbum(const QString& oldName, const QString& newName);
    void dbClearList();
    void dbGetAlbums();
    void dbCreateTables();
    void dbAddFiles(const QStringList& aFiles, const QString& album);
    void dbDelFiles(const QIntList& dbids);
    void dbGetFiles(const QString& album, const QString& search, Z::Ordering order, bool reverseOrder);
};

#endif // ZSEARCHTAB_H
