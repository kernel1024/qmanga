#include "zmangamodel.h"

ZMangaModel::ZMangaModel(QObject *parent, SQLMangaList *aList, int aPixmapSize) :
    QAbstractListModel(parent)
{
    list = aList;
    pixmapSize = aPixmapSize;
}

Qt::ItemFlags ZMangaModel::flags(const QModelIndex &) const
{
    return (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

QVariant ZMangaModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();

    if (index.row()>=list->length()) return QVariant();

    if (role == Qt::DecorationRole) {
        QPixmap p = list->at(index.row()).cover;
        if (!p.isNull()) {
            p = p.scaled(pixmapSize,pixmapSize,Qt::KeepAspectRatio,Qt::SmoothTransformation);
        } else
            p = QPixmap(":/img/edit-delete.png");
        return p;
    } else if (role == Qt::DisplayRole) {
        QString t = list->at(index.row()).name;
        return t;
    }
    return QVariant();
}

int ZMangaModel::rowCount(const QModelIndex &) const
{
    return list->count();
}

void ZMangaModel::setPixmapSize(int aPixmapSize)
{
    pixmapSize = aPixmapSize;
}
