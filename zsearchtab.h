#ifndef ZSEARCHTAB_H
#define ZSEARCHTAB_H

#include <QtCore>
#include <QtGui>
#include "zglobal.h"
#include "zsearchloader.h"

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
    void updateWidgetsState();
    
private:
    Ui::ZSearchTab *ui;
    SQLMangaList mangaList;
    QMutex listUpdating;
    bool loadingNow;
    QString descTemplate;
    ZSearchLoader loader;
    ZMangaModel* model;

    QSize gridSize(int ref);

public slots:
    void albumChanged(QListWidgetItem * current, QListWidgetItem * previous);
    void albumClicked(QListWidgetItem * item);
    void mangaClicked(const QModelIndex &index);
    void mangaOpen(const QModelIndex &index);
    void mangaAdd();
    void mangaAddDir();
    void mangaDel();
    void listModeChanged();
    void iconSizeChanged(int ref);
    void loaderFinished();
signals:
    void mangaDblClick(QString filename);
};

#endif // ZSEARCHTAB_H
