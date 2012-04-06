#ifndef ZMANGAMODEL_H
#define ZMANGAMODEL_H

#include <QAbstractListModel>
#include "zglobal.h"
#include "zdb.h"

class ZMangaModel : public QAbstractListModel
{
    Q_OBJECT
private:
    QSlider *pixmapSize;
    QListView *view;
public:
    explicit ZMangaModel(QObject *parent, QSlider *aPixmapSize, QListView *aView);

    Qt::ItemFlags flags(const QModelIndex & index) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    int rowCount( const QModelIndex & parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent) const;

    void setPixmapSize(QSlider *aPixmapSize);

public slots:
    void rowsAppended();
    void rowsAboutToAppended(int pos, int last);
    void rowsDeleted();
    void rowsAboutToDeleted(int pos, int last);
};

#endif // ZMANGAMODEL_H
