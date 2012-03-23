#include "zmangamodel.h"

ZMangaModel::ZMangaModel(QObject *parent, SQLMangaList *aList, QSlider* aPixmapSize, QListView* aView,
                         QMutex* aListUpdating) :
    QAbstractListModel(parent)
{
    mList = aList;
    pixmapSize = aPixmapSize;
    view = aView;
    listUpdating = aListUpdating;
}

Qt::ItemFlags ZMangaModel::flags(const QModelIndex &) const
{
    return (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

QVariant ZMangaModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || mList==NULL) return QVariant();
    int idx = index.row();

    listUpdating->lock();
    int maxl = mList->length();
    listUpdating->unlock();
    if (idx>=maxl)
        return QVariant();

    if (role == Qt::DecorationRole) {
        QFontMetrics fm(view->font());
        QPixmap rp;
        if (view->viewMode()==QListView::ListMode)
            rp = QPixmap(fm.height()*2,fm.height()*2);
        else
            rp = QPixmap(pixmapSize->value(),pixmapSize->value()*previewProps); // B4 paper proportions
        QPainter cp(&rp);
        cp.fillRect(0,0,rp.width(),rp.height(),QBrush(Qt::lightGray));
                    //view->palette().base());

        listUpdating->lock();
        qDebug() << mList->at(idx).name;
        QPixmap p = QPixmap::fromImage(mList->at(idx).cover);
        if (p.isNull())
            p = QPixmap(":/img/edit-delete.png");
        p.detach();
        listUpdating->unlock();

        p = p.scaled(rp.size(),Qt::KeepAspectRatio,Qt::SmoothTransformation);
        if (p.height()<rp.height())
            cp.drawPixmap(0,(rp.height()-p.height())/2,p);
        else
            cp.drawPixmap((rp.width()-p.width())/2,0,p);
        cp.setPen(QPen(Qt::lightGray));
        cp.drawLine(0,0,rp.width()-1,0);
        cp.drawLine(rp.width()-1,0,rp.width()-1,rp.height()-1);
        cp.drawLine(rp.width()-1,rp.height()-1,0,rp.height()-1);
        cp.drawLine(0,rp.height()-1,0,0);
        return rp;
    } else if (role == Qt::DisplayRole) {
        listUpdating->lock();
        QString t = mList->at(index.row()).name;
        listUpdating->unlock();
        return t;
    }
    return QVariant();
}

int ZMangaModel::rowCount(const QModelIndex &) const
{
    if (mList==NULL)
        return 0;
    listUpdating->lock();
    int cnt = mList->count();
    listUpdating->unlock();
    return cnt;
}

void ZMangaModel::setPixmapSize(QSlider* aPixmapSize)
{
    pixmapSize = aPixmapSize;
}

void ZMangaModel::rowsAppended()
{
    endInsertRows();
}

void ZMangaModel::rowsAboutToAppended(int pos, int last)
{
    beginInsertRows(QModelIndex(),pos,last);
}

void ZMangaModel::rowsDeleted()
{
    endRemoveRows();
}

void ZMangaModel::rowsAboutToDeleted(int pos, int last)
{
    beginRemoveRows(QModelIndex(),pos,last);
}
