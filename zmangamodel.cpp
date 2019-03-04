#include <QPainter>
#include <QPixmap>
#include <QPen>
#include <QScrollBar>
#include <QAbstractItemModel>
#include <QApplication>

#include "zmangamodel.h"

const int ModelSortRole = Qt::UserRole + 1;

ZMangaModel::ZMangaModel(QObject *parent, QSlider *aPixmapSize, QTableView *aView) :
    QAbstractTableModel(parent)
{
    pixmapSize = aPixmapSize;
    view = aView;
    mList.clear();
}

ZMangaModel::~ZMangaModel()
{
    mList.clear();
}

Qt::ItemFlags ZMangaModel::flags(const QModelIndex &) const
{
    return (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

QVariant ZMangaModel::data(const QModelIndex &index, int role) const
{
    return data(index,role,false);
}

QVariant ZMangaModel::data(const QModelIndex &index, int role, bool listMode) const
{
    if (!index.isValid() || zg==nullptr) return QVariant();
    int idx = index.row();

    int maxl = mList.count();
    if (idx<0 || idx>=maxl)
        return QVariant();

    QFontMetrics fm(view->font());
    if (role == Qt::DecorationRole && index.column()==0) {
        QPixmap rp;
        if (listMode)
            rp = QPixmap(fm.height()*2,fm.height()*2);
        else
            rp = QPixmap(pixmapSize->value(),pixmapSize->value()*previewProps); // B4 paper proportions
        QPainter cp(&rp);
        cp.fillRect(0,0,rp.width(),rp.height(),view->palette().base());

        SQLMangaEntry itm = mList.at(idx);
        QImage p = itm.cover;
        if (p.isNull())
            p = QImage(QStringLiteral(":/32/edit-delete"));

        p = p.scaled(rp.size(),Qt::KeepAspectRatio,Qt::SmoothTransformation);
        if (p.height()<rp.height())
            cp.drawImage(0,(rp.height()-p.height())/2,p);
        else
            cp.drawImage((rp.width()-p.width())/2,0,p);
        if (zg->frameColor!=zg->backgroundColor) {
            cp.setPen(QPen(zg->frameColor));
            cp.drawLine(0,0,rp.width()-1,0);
            cp.drawLine(rp.width()-1,0,rp.width()-1,rp.height()-1);
            cp.drawLine(rp.width()-1,rp.height()-1,0,rp.height()-1);
            cp.drawLine(0,rp.height()-1,0,0);
        }
        return rp;
    }

    if (role == Qt::TextColorRole) {
        return zg->foregroundColor();
    }

    if (role == Qt::FontRole) {
        return zg->idxFont;
    }

    if (role == Qt::DisplayRole) {
        SQLMangaEntry t = mList.at(idx);
        int col = index.column();
        QString tmp;
        int i;
        int maxLen = 0;
        switch (col) {
            case 0:
                // insert zero-width spaces for word-wrap
                tmp = t.name;
                i = tmp.length()-1;
                while (i>=0) {
                    if (!tmp.at(i).isLetterOrNumber())
                        tmp.insert(i,QChar(0x200b));
                    i--;
                }
                // QTBUG-16134 changes workaround - elided text is too big
                maxLen = qRound(3.5 * pixmapSize->value() / fm.averageCharWidth());
                if (!listMode && tmp.length()>maxLen) {
                    tmp.truncate(maxLen);
                    tmp.append(QStringLiteral("..."));
                }
                // -----
                return tmp;
            case 1: return t.album;
            case 2: return QStringLiteral("%1").arg(t.pagesCount);
            case 3: return formatSize(t.fileSize);
            case 4: return t.addingDT.toString(QStringLiteral("yyyy-MM-dd"));
            case 5: return t.fileDT.toString(QStringLiteral("yyyy-MM-dd"));
            case 6: return t.fileMagic;
        }
        return QVariant();
    }

    if (role == ModelSortRole) {
        SQLMangaEntry t = mList.at(idx);
        int col = index.column();
        switch (col) {
            case 0: return t.name;
            case 1: return t.album;
            case 2: return t.pagesCount;
            case 3: return t.fileSize;
            case 4: return t.addingDT;
            case 5: return t.fileDT;
            case 6: return t.fileMagic;
        }
        return QVariant();
    }

    if (role == Qt::ToolTipRole ||
               role == Qt::StatusTipRole) {
        return mList.at(idx).name;
    }

    return QVariant();
}

int ZMangaModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return mList.count();
}

int ZMangaModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return 7;
}

QVariant ZMangaModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal &&
            role == Qt::DisplayRole &&
            section>=0 && section<Z::maxOrdering) {
        auto s = static_cast<Z::Ordering>(section);
        if (Z::headerColumns.contains(s))
            return Z::headerColumns.value(s);

        return QVariant();
    }

    return QVariant();
}

int ZMangaModel::getItemsCount() const
{
    return mList.count();
}

SQLMangaEntry ZMangaModel::getItem(int idx) const
{
    if (idx<0 || idx>=mList.count())
        return SQLMangaEntry();

    return mList.at(idx);
}

void ZMangaModel::deleteAllItems()
{
    if (mList.isEmpty()) return;
    beginRemoveRows(QModelIndex(),0,mList.count()-1);
    mList.clear();
    endRemoveRows();
}

void ZMangaModel::deleteItems(const QIntVector &dbids)
{
    for (const auto &i : dbids) {
        int idx = mList.indexOf(SQLMangaEntry(i));
        if (idx>=0) {
            beginRemoveRows(QModelIndex(),idx,idx);
            mList.removeAt(idx);
            endRemoveRows();
        }
    }
}

void ZMangaModel::addItem(const SQLMangaEntry &file, const Z::Ordering sortOrder, const bool reverseOrder)
{
    Q_UNUSED(sortOrder)
    Q_UNUSED(reverseOrder)

    int posidx = mList.count();
    beginInsertRows(QModelIndex(),posidx,posidx);
    mList.append(file);
    endInsertRows();
}

ZMangaSearchHistoryModel::ZMangaSearchHistoryModel(QObject *parent)
    : QAbstractListModel(parent)
{

}

int ZMangaSearchHistoryModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return historyItems.count();
}

QVariant ZMangaSearchHistoryModel::data(const QModelIndex &index, int role) const
{
    int idx = index.row();

    if (idx<0 || idx>=historyItems.count()) return QVariant();

    if (role==Qt::DisplayRole || role==Qt::EditRole)
        return historyItems.at(idx);

    return QVariant();
}

Qt::ItemFlags ZMangaSearchHistoryModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)

    return (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

void ZMangaSearchHistoryModel::setHistoryItems(const QStringList &items)
{
    if (!historyItems.isEmpty()) {
        beginRemoveRows(QModelIndex(),0,historyItems.count()-1);
        historyItems.clear();
        endRemoveRows();
    }

    QStringList sl = items;
    sl.removeDuplicates();
    if (!sl.isEmpty()) {
        beginInsertRows(QModelIndex(),0,sl.count()-1);
        historyItems.append(sl);
        endInsertRows();
    }
}

QStringList ZMangaSearchHistoryModel::getHistoryItems() const
{
    return historyItems;
}

void ZMangaSearchHistoryModel::appendHistoryItem(const QString &item)
{
    if (historyItems.contains(item)) return;

    beginInsertRows(QModelIndex(),historyItems.count(),historyItems.count());
    historyItems.append(item);
    endInsertRows();
}

ZMangaListView::ZMangaListView(QWidget *parent)
    : QListView(parent)
{

}

void ZMangaListView::updateGeometries()
{
    QListView::updateGeometries();
    if (verticalScrollBar()!=nullptr)
        verticalScrollBar()->setSingleStep(
                    static_cast<int>(static_cast<double>(gridSize().height())*zg->searchScrollFactor));
}

ZMangaTableModel::ZMangaTableModel(QObject *parent, QTableView *aView)
    : QSortFilterProxyModel(parent)
{
    view = aView;
    setSortRole(ModelSortRole);
}

int ZMangaTableModel::columnCount(const QModelIndex &) const
{
    if (zg!=nullptr) {
        if (view->palette().base().color()!=zg->backgroundColor) {
            QPalette pl = view->palette();
            pl.setBrush(QPalette::Base,QBrush(zg->backgroundColor));
            view->setPalette(pl);
        }
    }
    return 7;
}

QVariant ZMangaTableModel::data(const QModelIndex &index, int role) const
{
    auto model = qobject_cast<ZMangaModel *>(sourceModel());
    return model->data(mapToSource(index),role,true);
}

ZMangaIconModel::ZMangaIconModel(QObject *parent, ZMangaListView *aView)
    : QSortFilterProxyModel(parent)
{
    view = aView;
    setSortRole(ModelSortRole);
}

int ZMangaIconModel::columnCount(const QModelIndex &) const
{
    if (zg!=nullptr) {
        if (view->palette().base().color()!=zg->backgroundColor) {
            QPalette pl = view->palette();
            pl.setBrush(QPalette::Base,QBrush(zg->backgroundColor));
            view->setPalette(pl);
        }
    }
    return 1;
}
