#ifndef ZMANGAMODEL_H
#define ZMANGAMODEL_H

#include <QSlider>
#include <QListView>
#include <QAbstractListModel>
#include <QStyledItemDelegate>
#include <QHeaderView>
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
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole, int columnOverride = -1) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    int rowCount( const QModelIndex & parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    void setPixmapSize(QSlider *aPixmapSize);
    int getItemsCount();
    SQLMangaEntry getItem(int idx);

public slots:
    void deleteAllItems();
    void deleteItems(const QIntList& dbids);
    void addItem(const SQLMangaEntry& file);
};

class ZMangaListView : public QListView {
    Q_OBJECT
protected:
    void resizeEvent(QResizeEvent *e);

public:
    QHeaderView* header;

    ZMangaListView(QWidget *parent = 0);
    ~ZMangaListView();
    void setModel(QAbstractItemModel *model);
    void setViewMode(ViewMode mode);

private:
    int headerHeight;

public slots:
    void updateWidths(int logicalIndex, int oldSize, int newSize);
};

class ZMangaListItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
private:
    ZMangaListView *view;
    ZMangaModel *model;
public:
    explicit ZMangaListItemDelegate(QObject *parent, ZMangaListView *aView, ZMangaModel *aModel);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class ZMangaListHeaderView : public QHeaderView {
    Q_OBJECT
public:
    ZMangaListHeaderView(Qt::Orientation orientation, QListView *parent = 0);
    QSize sizeHint();
};

class ZMangaSearchHistoryModel : public QAbstractListModel {
    Q_OBJECT
private:
    QStringList historyItems;
public:
    ZMangaSearchHistoryModel(QObject *parent = 0);
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    void setHistoryItems(const QStringList& items);
    QStringList getHistoryItems() const;
    void appendHistoryItem(const QString& item);
};

#endif // ZMANGAMODEL_H
