#ifndef ZMANGAMODEL_H
#define ZMANGAMODEL_H

#include <QAbstractListModel>
#include "global.h"
#include "zglobal.h"
#include "zdb.h"

class ZMangaModel : public QAbstractListModel
{
    Q_OBJECT
private:
    QSlider *pixmapSize;
    QListView *view;

    SQLMangaList mList;

public:
    explicit ZMangaModel(QObject *parent, QSlider *aPixmapSize, QListView *aView);
    ~ZMangaModel();

    Qt::ItemFlags flags(const QModelIndex & index) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    int rowCount( const QModelIndex & parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent) const;

    void setPixmapSize(QSlider *aPixmapSize);
    int getItemsCount();
    SQLMangaEntry getItem(int idx);

public slots:
    void deleteAllItems();
    void deleteItems(const QIntList& dbids);
    void addItem(const SQLMangaEntry& file);
};

#endif // ZMANGAMODEL_H
