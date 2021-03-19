#include <QPainter>
#include <QPixmap>
#include <QPen>
#include <QScrollBar>
#include <QDropEvent>
#include <QAbstractItemModel>
#include <QApplication>

#include "zmangamodel.h"

namespace ZDefaults {
constexpr int ModelSortRole = Qt::UserRole + 1;
const int columnName = 0;
const int columnAlbum = 1;
const int columnPagesCount = 2;
const int columnFileSize = 3;
const int columnAddDate = 4;
const int columnFileDate = 5;
const int columnMagic = 6;
const int columnsCount = 7;
const double textWidthFactor = 3.5;
}

ZMangaModel::ZMangaModel(QObject *parent, QSlider *aPixmapSize, QTableView *aView) :
    QAbstractTableModel(parent)
{
    m_pixmapSize = aPixmapSize;
    m_view = aView;
}

ZMangaModel::~ZMangaModel()
{
    m_list.clear();
}

Qt::ItemFlags ZMangaModel::flags(const QModelIndex &index) const
{
    if (!checkIndex(index,CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
        return Qt::NoItemFlags;

    return (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

QVariant ZMangaModel::data(const QModelIndex &index, int role) const
{
    return data(index,role,false);
}

QVariant ZMangaModel::data(const QModelIndex &index, int role, bool listMode) const
{
    const QChar zeroWidthSpace(0x200b);

    if (zF->global()==nullptr) return QVariant();
    if (!checkIndex(index,CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
        return QVariant();

    int idx = index.row();

    QFontMetrics fm(m_view->font());
    if (role == Qt::DecorationRole && index.column()==0) {
        QPixmap rp;
        if (listMode) {
            rp = QPixmap(fm.height()*2,fm.height()*2);
        } else {
            rp = QPixmap(m_pixmapSize->value(),qRound(m_pixmapSize->value()*ZDefaults::previewProps));
        }
        QPainter cp(&rp);
        cp.fillRect(0,0,rp.width(),rp.height(),m_view->palette().base());

        ZSQLMangaEntry itm = m_list.at(idx);
        QImage p = itm.cover;
        if (p.isNull())
            p = QImage(QSL(":/32/edit-delete"));

        p = p.scaled(rp.size(),Qt::KeepAspectRatio,Qt::SmoothTransformation);

        if (p.height()<rp.height()) {
            cp.drawImage(0,(rp.height()-p.height())/2,p);
        } else {
            cp.drawImage((rp.width()-p.width())/2,0,p);
        }
        if (zF->global()->getFrameColor()!=zF->global()->getBackgroundColor()) {
            cp.setPen(QPen(zF->global()->getFrameColor()));
            cp.drawLine(0,0,rp.width()-1,0);
            cp.drawLine(rp.width()-1,0,rp.width()-1,rp.height()-1);
            cp.drawLine(rp.width()-1,rp.height()-1,0,rp.height()-1);
            cp.drawLine(0,rp.height()-1,0,0);
        }
        return rp;
    }

    if (role == Qt::ForegroundRole) {
        return zF->global()->getForegroundColor();
    }

    if (role == Qt::FontRole) {
        return zF->global()->getIdxFont();
    }

    if (role == Qt::DisplayRole) {
        ZSQLMangaEntry t = m_list.at(idx);
        int col = index.column();
        QString tmp;
        int i = 0;
        int maxLen = 0;
        switch (col) {
            case ZDefaults::columnName:
                // insert zero-width spaces for word-wrap
                tmp = t.name;
                i = tmp.length()-1;
                while (i>=0) {
                    if (!tmp.at(i).isLetterOrNumber())
                        tmp.insert(i,zeroWidthSpace);
                    i--;
                }
                // QTBUG-16134 changes workaround - elided text is too big
                maxLen = qRound(ZDefaults::textWidthFactor * m_pixmapSize->value() / fm.averageCharWidth());
                if (!listMode && tmp.length()>maxLen) {
                    tmp.truncate(maxLen);
                    tmp.append(QSL("..."));
                }
                // -----
                return tmp;
            case ZDefaults::columnAlbum: return t.album;
            case ZDefaults::columnPagesCount: return QSL("%1").arg(t.pagesCount);
            case ZDefaults::columnFileSize: return zF->formatSize(t.fileSize);
            case ZDefaults::columnAddDate: return t.addingDT.toString(QSL("yyyy-MM-dd"));
            case ZDefaults::columnFileDate: return t.fileDT.toString(QSL("yyyy-MM-dd"));
            case ZDefaults::columnMagic: return t.fileMagic;
            default: return QVariant();
        }
    }

    if (role == ZDefaults::ModelSortRole) {
        ZSQLMangaEntry t = m_list.at(idx);
        int col = index.column();
        switch (col) {
            case ZDefaults::columnName: return t.name;
            case ZDefaults::columnAlbum: return t.album;
            case ZDefaults::columnPagesCount: return t.pagesCount;
            case ZDefaults::columnFileSize: return t.fileSize;
            case ZDefaults::columnAddDate: return t.addingDT;
            case ZDefaults::columnFileDate: return t.fileDT;
            case ZDefaults::columnMagic: return t.fileMagic;
            default: return QVariant();
        }
    }

    if (role == Qt::ToolTipRole ||
               role == Qt::StatusTipRole) {
        return m_list.at(idx).name;
    }

    return QVariant();
}

int ZMangaModel::rowCount(const QModelIndex &parent) const
{
    if (!checkIndex(parent))
        return 0;
    if (parent.isValid())
        return 0;

    return m_list.count();
}

int ZMangaModel::columnCount(const QModelIndex &parent) const
{
    if (!checkIndex(parent))
        return 0;
    if (parent.isValid())
        return 0;

    return ZDefaults::columnsCount;
}

QVariant ZMangaModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal &&
            role == Qt::DisplayRole &&
            section>=0 && section<Z::maxOrdering) {
        auto s = static_cast<Z::Ordering>(section);
        if (ZGenericFuncs::getHeaderColumns().contains(s))
            return ZGenericFuncs::getHeaderColumns().value(s);

        return QVariant();
    }

    return QVariant();
}

int ZMangaModel::getItemsCount() const
{
    return m_list.count();
}

ZSQLMangaEntry ZMangaModel::getItem(int idx) const
{
    if (idx<0 || idx>=m_list.count())
        return ZSQLMangaEntry();

    return m_list.at(idx);
}

void ZMangaModel::deleteAllItems()
{
    if (m_list.isEmpty()) return;
    beginRemoveRows(QModelIndex(),0,m_list.count()-1);
    m_list.clear();
    endRemoveRows();
}

void ZMangaModel::deleteItems(const ZIntVector &dbids)
{
    for (const auto &i : dbids) {
        int idx = m_list.indexOf(ZSQLMangaEntry(i));
        if (idx>=0) {
            beginRemoveRows(QModelIndex(),idx,idx);
            m_list.removeAt(idx);
            endRemoveRows();
        }
    }
}

void ZMangaModel::addItem(const ZSQLMangaEntry &file)
{
    int posidx = m_list.count();
    beginInsertRows(QModelIndex(),posidx,posidx);
    m_list.append(file);
    endInsertRows();
}

ZMangaSearchHistoryModel::ZMangaSearchHistoryModel(QObject *parent)
    : QAbstractListModel(parent)
{

}

int ZMangaSearchHistoryModel::rowCount(const QModelIndex &parent) const
{
    if (!checkIndex(parent))
        return 0;
    if (parent.isValid())
        return 0;

    return m_historyItems.count();
}

QVariant ZMangaSearchHistoryModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index,CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
        return QVariant();

    int idx = index.row();

    if (role==Qt::DisplayRole || role==Qt::EditRole)
        return m_historyItems.at(idx);

    return QVariant();
}

Qt::ItemFlags ZMangaSearchHistoryModel::flags(const QModelIndex &index) const
{
    if (!checkIndex(index,CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
        return Qt::NoItemFlags;

    return (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

void ZMangaSearchHistoryModel::setHistoryItems(const QStringList &items)
{
    if (!m_historyItems.isEmpty()) {
        beginRemoveRows(QModelIndex(),0,m_historyItems.count()-1);
        m_historyItems.clear();
        endRemoveRows();
    }

    QStringList sl = items;
    sl.removeDuplicates();
    if (!sl.isEmpty()) {
        beginInsertRows(QModelIndex(),0,sl.count()-1);
        m_historyItems.append(sl);
        endInsertRows();
    }
}

QStringList ZMangaSearchHistoryModel::getHistoryItems() const
{
    return m_historyItems;
}

void ZMangaSearchHistoryModel::appendHistoryItem(const QString &item)
{
    if (m_historyItems.contains(item)) return;

    beginInsertRows(QModelIndex(),m_historyItems.count(),m_historyItems.count());
    m_historyItems.append(item);
    endInsertRows();
}

ZMangaListView::ZMangaListView(QWidget *parent)
    : QListView(parent)
{

}

void ZMangaListView::updateGeometries()
{
    QListView::updateGeometries();
    if (verticalScrollBar()!=nullptr) {
        verticalScrollBar()->setSingleStep(
                    static_cast<int>(static_cast<double>(gridSize().height())
                                     *zF->global()->getSearchScrollFactor()));
    }
}

ZMangaTableModel::ZMangaTableModel(QObject *parent, QTableView *aView)
    : QSortFilterProxyModel(parent)
{
    m_view = aView;
    setSortRole(ZDefaults::ModelSortRole);
}

int ZMangaTableModel::columnCount(const QModelIndex &parent) const
{
    if (!checkIndex(parent))
        return 0;
    if (parent.isValid())
        return 0;

    if (zF->global()!=nullptr) {
        if (m_view->palette().base().color()!=zF->global()->getBackgroundColor()) {
            QPalette pl = m_view->palette();
            pl.setBrush(QPalette::Base,QBrush(zF->global()->getBackgroundColor()));
            m_view->setPalette(pl);
        }
    }
    return ZDefaults::columnsCount;
}

QVariant ZMangaTableModel::data(const QModelIndex &index, int role) const
{
    auto *model = qobject_cast<ZMangaModel *>(sourceModel());
    return model->data(mapToSource(index),role,true);
}

ZMangaIconModel::ZMangaIconModel(QObject *parent, ZMangaListView *aView)
    : QSortFilterProxyModel(parent)
{
    m_view = aView;
    setSortRole(ZDefaults::ModelSortRole);
}

int ZMangaIconModel::columnCount(const QModelIndex &parent) const
{
    if (!checkIndex(parent))
        return 0;
    if (parent.isValid())
        return 0;

    if (zF->global()!=nullptr) {
        if (m_view->palette().base().color()!=zF->global()->getBackgroundColor()) {
            QPalette pl = m_view->palette();
            pl.setBrush(QPalette::Base,QBrush(zF->global()->getBackgroundColor()));
            m_view->setPalette(pl);
        }
    }
    const int columnsCount = 1;
    return columnsCount;
}

ZAlbumsTreeWidget::ZAlbumsTreeWidget(QWidget *parent)
    : QTreeWidget (parent)
{
    setSelectionMode(QAbstractItemView::SingleSelection);
    setDragEnabled(true);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::InternalMove);
}

void ZAlbumsTreeWidget::dragEnterEvent(QDragEnterEvent *event)
{
    m_draggingItem = currentIndex();
    if (!m_draggingItem.isValid()) {
        event->ignore();
    }
    QTreeWidgetItem* src = itemFromIndex(m_draggingItem);
    if (!src) {
        event->ignore();
        return;
    }
    int srcId = src->data(0,Qt::UserRole).toInt();
    if (srcId<0) {
        event->ignore();
        return;
    }

    QTreeWidget::dragEnterEvent(event);
}

void ZAlbumsTreeWidget::dropEvent(QDropEvent *event)
{
    QModelIndex droppedIndex = indexAt(event->pos());
    if( !droppedIndex.isValid() || !m_draggingItem.isValid()) {
        event->ignore();
        return;
    }

    QTreeWidgetItem* src = itemFromIndex(m_draggingItem);
    QTreeWidgetItem* dst = itemFromIndex(droppedIndex);
    if (!src || !dst) {
        event->ignore();
        return;
    }

    int srcId = src->data(0,Qt::UserRole).toInt();
    int dstId = dst->data(0,Qt::UserRole).toInt();
    if (srcId<0 || dstId<0) {  // no dragging between virtual albums
        event->ignore();
        return;
    }

    const QString srcName = src->text(0);
    const QString dstName = dst->text(0);

    QTimer::singleShot(0,zF->global()->db(),[srcName,dstName](){
        if (zF->global()->db())
            zF->global()->db()->sqlReparentAlbum(srcName,dstName);
    });
    event->accept();
}
