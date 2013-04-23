#ifndef ZSEARCHTAB_H
#define ZSEARCHTAB_H

#include <QWidget>
#include <QSlider>
#include <QRadioButton>
#include <QProgressDialog>
#include <QListWidget>
#include <QListWidgetItem>

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

    explicit ZSearchTab(QWidget *parent = 0);
    ~ZSearchTab();

    void updateAlbumsList();
    void applyOrder(Z::Ordering aOrder, bool aReverseOrder, bool updateGUI = false);
    
private:
    Ui::ZSearchTab *ui;
    QStateMachine loadingState;
    QString descTemplate;
    QPointer<ZMangaModel> model;
    QProgressDialog progressDlg;
    QStringList cachedAlbums;
    Z::Ordering order;
    bool reverseOrder;

    QSize gridSize(int ref);
    QString getAlbumNameToAdd(QString suggest);

public slots:
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
    void ctxAlbumMenu(QPoint pos);
    void ctxSorting();
    void ctxRenameAlbum();
    void ctxDeleteAlbum();
    void ctxOpenDir();

    void dbShowProgressDialog(const bool visible);
    void dbShowProgressState(const int value, const QString& msg);
    void dbAlbumsListUpdated();
    void dbAlbumsListReady(const QStringList& albums);
    void dbFilesAdded(const int count, const int total, const int elapsed);
    void dbFilesLoaded(const int count, const int elapsed);
    void dbErrorMsg(const QString& msg);
    void dbNeedTableCreation();

signals:
    void mangaDblClick(QString filename);
    void statusBarMsg(QString msg);
    void dbRenameAlbum(const QString& oldName, const QString& newName);
    void dbGetAlbums();
    void dbCreateTables();
    void dbAddFiles(const QStringList& aFiles, const QString& album);
    void dbGetFiles(const QString& album, const QString& search, const int order, const bool reverseOrder);
    void dbDelFiles(const QIntList& dbids);
    void dbDeleteAlbum(const QString& album);
};

#endif // ZSEARCHTAB_H
