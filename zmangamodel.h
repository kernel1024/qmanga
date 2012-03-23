#ifndef ZMANGAMODEL_H
#define ZMANGAMODEL_H

#include <QAbstractListModel>
#include "zglobal.h"

class ZMangaModel : public QAbstractListModel
{
    Q_OBJECT
private:
    SQLMangaList *mList;
    QSlider *pixmapSize;
    QListView *view;
    QMutex *listUpdating;
public:
    explicit ZMangaModel(QObject *parent, SQLMangaList *aList, QSlider *aPixmapSize,
                         QListView *aView, QMutex *aListUpdating);

    Qt::ItemFlags flags(const QModelIndex & index) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    int rowCount( const QModelIndex & parent = QModelIndex()) const;

    void setPixmapSize(QSlider *aPixmapSize);

public slots:
    void rowsAppended();
    void rowsAboutToAppended(int pos, int last);
    void rowsDeleted();
    void rowsAboutToDeleted(int pos, int last);
};

#endif // ZMANGAMODEL_H
