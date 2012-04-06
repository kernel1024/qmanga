#include "zmangamodel.h"

ZMangaModel::ZMangaModel(QObject *parent, QSlider *aPixmapSize, QListView *aView) :
    QAbstractListModel(parent)
{
    pixmapSize = aPixmapSize;
    view = aView;
}

Qt::ItemFlags ZMangaModel::flags(const QModelIndex &) const
{
    return (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

QVariant ZMangaModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || zg==NULL || zg->db==NULL) return QVariant();
    int idx = index.row();

    int maxl = zg->db->getResultCount();
    if (idx<0 || idx>=maxl)
        return QVariant();

    if (role == Qt::DecorationRole && index.column()==0) {
        QFontMetrics fm(view->font());
        QPixmap rp;
        if (view->viewMode()==QListView::ListMode)
            rp = QPixmap(fm.height()*2,fm.height()*2);
        else
            rp = QPixmap(pixmapSize->value(),pixmapSize->value()*previewProps); // B4 paper proportions
        QPainter cp(&rp);
        cp.fillRect(0,0,rp.width(),rp.height(),view->palette().base());

        SQLMangaEntry itm = zg->db->getResultItem(idx);
        QPixmap p = QPixmap::fromImage(itm.cover);
        if (p.isNull())
            p = QPixmap(":/img/edit-delete.png");
        p.detach();

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
    } else if (role == Qt::TextColorRole) {
        return zg->foregroundColor();
    } else if (role == Qt::FontRole) {
        return zg->idxFont;
    } else if (role == Qt::DisplayRole) {
        SQLMangaEntry t = zg->db->getResultItem(idx);
        switch (index.column()) {
            case 0: return t.name;
            case 1: return t.album;
            case 2: return QString("%1").arg(t.pagesCount);
            case 3: return t.addingDT.toString("yyyy-MM-dd");
            case 4: return t.fileDT.toString("yyyy-MM-dd");
            default: return QVariant();
        }
        return QVariant();
    } else if (role == Qt::ToolTipRole || role == Qt::StatusTipRole) {
        SQLMangaEntry t = zg->db->getResultItem(idx);
        return t.name;
    }
    return QVariant();
}

QVariant ZMangaModel::headerData(int section, Qt::Orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        switch (section) {
            case 0: return tr("Name");
            case 1: return tr("Album");
            case 2: return tr("Pages");
            case 3: return tr("Added");
            case 4: return tr("Created");
            default: return QVariant();
        }
        return QVariant();
    }
    return QVariant();
}

int ZMangaModel::rowCount(const QModelIndex &) const
{
    if (zg==NULL) return 0;
    if (view->palette().base().color()!=zg->backgroundColor) {
        QPalette pl = view->palette();
        pl.setBrush(QPalette::Base,QBrush(zg->backgroundColor));
        view->setPalette(pl);
    }

    if (zg->db==NULL) return 0;
    return zg->db->getResultCount();
}

int ZMangaModel::columnCount(const QModelIndex &) const
{
    return 5;
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
