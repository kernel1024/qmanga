#ifndef ZSEARCHTAB_H
#define ZSEARCHTAB_H

#include <QtCore>
#include <QtGui>
#include "zglobal.h"

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
    
private:
    Ui::ZSearchTab *ui;
    SQLMangaList mangaList;

public slots:
    void albumChanged(QListWidgetItem * current, QListWidgetItem * previous);
    void mangaClicked(const QModelIndex &index);
    void mangaOpen(const QModelIndex &index);
    void mangaAdd();
    void mangaDel();
signals:
    void mangaDblClick(QString filename);
};

#endif // ZSEARCHTAB_H
