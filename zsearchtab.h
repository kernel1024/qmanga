#ifndef ZSEARCHTAB_H
#define ZSEARCHTAB_H

#include <QtCore>
#include <QtGui>
#include "zglobal.h"
#include "zsearchloader.h"
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
    ZGlobal::ZOrdering order;
    bool reverseOrder;

    explicit ZSearchTab(QWidget *parent = 0);
    ~ZSearchTab();

    void updateAlbumsList();
    void updateWidgetsState();
    
private:
    Ui::ZSearchTab *ui;
    SQLMangaList mangaList;
    QMutex listUpdating;
    bool loadingNow;
    QString descTemplate;
    ZSearchLoader loader;
    ZMangaModel* model;
    QTime searchTimer;

    QSize gridSize(int ref);

public slots:
    void albumChanged(QListWidgetItem * current, QListWidgetItem * previous);
    void albumClicked(QListWidgetItem * item);
    void mangaSearch();
    void mangaClicked(const QModelIndex &index);
    void mangaOpen(const QModelIndex &index);
    void mangaAdd();
    void mangaAddDir();
    void mangaDel();
    void listModeChanged(bool state);
    void iconSizeChanged(int ref);
    void loaderFinished();
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
signals:
    void mangaDblClick(QString filename);
    void statusBarMsg(QString msg);
};

#endif // ZSEARCHTAB_H
