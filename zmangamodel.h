#ifndef ZMANGAMODEL_H
#define ZMANGAMODEL_H

#include <QAbstractListModel>
#include "zglobal.h"

class ZMangaModel : public QAbstractListModel
{
    Q_OBJECT
private:
    SQLMangaList *list;
    int pixmapSize;
public:
    explicit ZMangaModel(QObject *parent, SQLMangaList *aList, int aPixmapSize);
    Qt::ItemFlags flags(const QModelIndex & index) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    int rowCount( const QModelIndex & parent = QModelIndex()) const;
    void setPixmapSize(int aPixmapSize);
};

#endif // ZMANGAMODEL_H
