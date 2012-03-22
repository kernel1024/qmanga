#include "zmangamodel.h"

ZMangaModel::ZMangaModel(QObject *parent, SQLMangaList *aList, QSlider* aPixmapSize, QListView* aView) :
    QAbstractListModel(parent)
{
    list = aList;
    pixmapSize = aPixmapSize;
    view = aView;
}

Qt::ItemFlags ZMangaModel::flags(const QModelIndex &) const
{
    return (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

QVariant ZMangaModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || list==NULL) return QVariant();
    if (index.row()>=list->length()) return QVariant();

    if (role == Qt::DecorationRole) {
        QPixmap p = list->at(index.row()).cover;
        if (!p.isNull()) {
            if (view->viewMode()==QListView::ListMode) {
                QFontMetrics fm(view->font());
                p = p.scaled(fm.height()*2,fm.height()*2,Qt::KeepAspectRatio,Qt::SmoothTransformation);
            } else
                p = p.scaled(pixmapSize->value(),pixmapSize->value(),Qt::KeepAspectRatio,Qt::SmoothTransformation);
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
    if (list==NULL)
        return 0;
    else
        return list->count();
}

void ZMangaModel::setPixmapSize(QSlider* aPixmapSize)
{
    pixmapSize = aPixmapSize;
}
