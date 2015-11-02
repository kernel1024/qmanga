#include <QPainter>
#include <QPixmap>
#include <QPen>
#include <QScrollBar>
#include <QAbstractItemModel>
#include <QApplication>

#include "zmangamodel.h"

ZMangaModel::ZMangaModel(QObject *parent, QSlider *aPixmapSize, QListView *aView) :
    QAbstractListModel(parent)
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
    return data(index,role,-1);
}

QVariant ZMangaModel::data(const QModelIndex &index, int role, int columnOverride) const
{
    if (!index.isValid() || zg==NULL) return QVariant();
    int idx = index.row();

    int maxl = mList.count();
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

        SQLMangaEntry itm = mList.at(idx);
        QPixmap p = QPixmap::fromImage(itm.cover);
        if (p.isNull())
            p = QPixmap(":/img/edit-delete.png");
        p.detach();

        p = p.scaled(rp.size(),Qt::KeepAspectRatio,Qt::SmoothTransformation);
        if (p.height()<rp.height())
            cp.drawPixmap(0,(rp.height()-p.height())/2,p);
        else
            cp.drawPixmap((rp.width()-p.width())/2,0,p);
        if (zg->frameColor!=zg->backgroundColor) {
            cp.setPen(QPen(zg->frameColor));
            cp.drawLine(0,0,rp.width()-1,0);
            cp.drawLine(rp.width()-1,0,rp.width()-1,rp.height()-1);
            cp.drawLine(rp.width()-1,rp.height()-1,0,rp.height()-1);
            cp.drawLine(0,rp.height()-1,0,0);
        }
        return rp;
    } else if (role == Qt::TextColorRole) {
        return zg->foregroundColor();
    } else if (role == Qt::FontRole) {
        return zg->idxFont;
    } else if (role == Qt::DisplayRole) {
        SQLMangaEntry t = mList.at(idx);
        int col = index.column();
        if (columnOverride>=0)
            col = columnOverride;
        switch (col) {
            case 0: return t.name;
            case 1: return QString("%1").arg(t.pagesCount);
            case 2: return formatSize(t.fileSize);
            case 3: return t.addingDT.toString("yyyy-MM-dd");
            case 4: return t.fileDT.toString("yyyy-MM-dd");
            default: return QVariant();
        }
        return QVariant();
    } else if (role == Qt::ToolTipRole ||
               role == Qt::StatusTipRole) {
        return mList.at(idx).name;
    }
    return QVariant();
}

int ZMangaModel::rowCount(const QModelIndex &) const
{
    if (zg==NULL) return mList.count();

    if (view->palette().base().color()!=zg->backgroundColor) {
        QPalette pl = view->palette();
        pl.setBrush(QPalette::Base,QBrush(zg->backgroundColor));
        view->setPalette(pl);
    }

    return mList.count();
}

int ZMangaModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return 5;
}

QVariant ZMangaModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation)

    if (role == Qt::DisplayRole) {
        switch (section) {
            case 0: return tr("Name");
            case 1: return tr("Pages");
            case 2: return tr("Size");
            case 3: return tr("Added");
            case 4: return tr("Created");
            default: return QVariant();
        }
        return QVariant();
    }
    return QVariant();
}

void ZMangaModel::setPixmapSize(QSlider* aPixmapSize)
{
    pixmapSize = aPixmapSize;
}

int ZMangaModel::getItemsCount()
{
    return mList.count();
}

SQLMangaEntry ZMangaModel::getItem(int idx)
{
    if (idx<0 || idx>=mList.count())
        return SQLMangaEntry();

    return mList.at(idx);
}

void ZMangaModel::deleteAllItems()
{
    beginRemoveRows(QModelIndex(),0,mList.count()-1);
    mList.clear();
    endRemoveRows();
}

void ZMangaModel::deleteItems(const QIntList &dbids)
{
    for (int i=0;i<dbids.count();i++) {
        int idx = mList.indexOf(SQLMangaEntry(dbids.at(i)));
        if (idx>=0) {
            beginRemoveRows(QModelIndex(),idx,idx);
            mList.removeAt(idx);
            endRemoveRows();
        }
    }
}

void ZMangaModel::addItem(const SQLMangaEntry &file)
{
    int posidx = mList.count();
    beginInsertRows(QModelIndex(),posidx,posidx);
    mList.append(file);
    endInsertRows();
    if (view->verticalScrollBar()!=NULL)
        view->verticalScrollBar()->setSingleStep(view->verticalScrollBar()->pageStep()/2);
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
    beginRemoveRows(QModelIndex(),0,historyItems.count()-1);
    historyItems.clear();
    endRemoveRows();

    QStringList sl = items;
    sl.removeDuplicates();
    beginInsertRows(QModelIndex(),0,sl.count()-1);
    historyItems.append(sl);
    endInsertRows();
}

QStringList ZMangaSearchHistoryModel::getHistoryItems() const
{
    return historyItems;
}

void ZMangaSearchHistoryModel::appendHistoryItem(const QString &item)
{
    if (historyItems.contains(item)) return;

    beginInsertRows(QModelIndex(),historyItems.count()-1,historyItems.count());
    historyItems.append(item);
    endInsertRows();
}

ZMangaListItemDelegate::ZMangaListItemDelegate(QObject *parent, ZMangaListView *aView, ZMangaModel *aModel)
    : QStyledItemDelegate(parent)
{
    view = aView;
    model = aModel;
}

void ZMangaListItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    if (view->viewMode()==QListView::ListMode) {

        QStyle *style = view->style();

        int widthAcc = 0;

        QStyleOptionViewItemV4 opt = option;
        initStyleOption( &opt, index );

        QRect srctr = opt.rect;
        QString text = opt.text;
        opt.text.clear();
        // draw background for entire row, overpaint later with text from additional columns
        style->drawControl( QStyle::CE_ItemViewItem, &opt, painter, view );

        opt.text = text;
        opt.rect.setWidth(view->header->sectionSize(0));
        style->drawControl( QStyle::CE_ItemViewItem, &opt, painter, view );

        widthAcc += view->header->sectionSize(0);

        painter->save();

        bool is_enabled = opt.state & QStyle::State_Enabled;
        QPalette::ColorGroup cg = is_enabled ? QPalette::Normal : QPalette::Disabled;
        if( cg == QPalette::Normal && !( opt.state & QStyle::State_Active ) )
            cg = QPalette::Inactive;

        if( opt.state & QStyle::State_Selected )
            painter->setPen( opt.palette.color( cg, QPalette::HighlightedText) );
        else
            painter->setPen( opt.palette.color( cg, QPalette::Text ) );

        for (int i=1;i<model->columnCount(QModelIndex());i++) {

            QRect tr = srctr;
            tr.setWidth(view->header->sectionSize(i));
            tr.translate(widthAcc,0);
            widthAcc += tr.width();

            QString s = model->data(index,Qt::DisplayRole,i).toString();
            painter->drawText(tr, Qt::AlignCenter | Qt::AlignVCenter, s);

        }

        painter->restore();

    } else
        QStyledItemDelegate::paint(painter, option, index);
}

ZMangaListHeaderView::ZMangaListHeaderView(Qt::Orientation orientation, QListView *parent)
    : QHeaderView(orientation, parent)
{

}

QSize ZMangaListHeaderView::sizeHint()
{
    QSize sz = QHeaderView::sizeHint();
    QListView* lv = qobject_cast<QListView *>(parent());
    if (lv!=NULL) {
        QFontMetrics fm(lv->font());
        sz.setHeight(4*fm.height()/3);
    }
    return sz;
}

void ZMangaListView::resizeEvent(QResizeEvent *e)
{
    Q_UNUSED(e)

    if (header->isVisible()) {
        setViewportMargins(0,headerHeight,0,0);

        QRect hg = viewport()->geometry();
        header->setGeometry(hg.topLeft().x(), hg.topLeft().y()-headerHeight, hg.width(), headerHeight);
    } else
        setViewportMargins(0,0,0,0);
}

ZMangaListView::ZMangaListView(QWidget *parent)
    : QListView(parent)
{
    QFontMetrics fm(QApplication::font());
    headerHeight = 6*fm.height()/5;
    header = new QHeaderView(Qt::Horizontal, this);
    header->hide();

    connect(header, SIGNAL(sectionResized(int,int,int)), this, SLOT(updateWidths(int,int,int)));
}

ZMangaListView::~ZMangaListView()
{
    if (header!=NULL)
        header->deleteLater();
}

void ZMangaListView::setModel(QAbstractItemModel *model)
{
    QListView::setModel(model);
    header->setModel(model);

}

void ZMangaListView::setViewMode(QListView::ViewMode mode)
{
    QListView::setViewMode(mode);
    header->setVisible(mode==ListMode);
    if (header->isVisible())
        setViewportMargins(0,headerHeight,0,0);
    else
        setViewportMargins(0,0,0,0);
}

void ZMangaListView::updateWidths(int logicalIndex, int oldSize, int newSize)
{
    Q_UNUSED(logicalIndex)
    Q_UNUSED(oldSize)
    Q_UNUSED(newSize)

    ZMangaModel* m = qobject_cast<ZMangaModel *>(model());
    if (m==NULL) return;

    for (int i=0;i<m->rowCount();i++)
        update(m->index(i));
}
