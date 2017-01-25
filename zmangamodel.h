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

class ZMangaListView;

class ZMangaModel : public QAbstractListModel
{
    Q_OBJECT
private:
    QSlider *pixmapSize;
    ZMangaListView *view;

    SQLMangaList mList;

public:
    explicit ZMangaModel(QObject *parent, QSlider *aPixmapSize, ZMangaListView *aView);
    ~ZMangaModel();

    Qt::ItemFlags flags(const QModelIndex & index) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole, int columnOverride = -1) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    int rowCount( const QModelIndex & parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    void setPixmapSize(QSlider *aPixmapSize);
    int getItemsCount() const;
    SQLMangaEntry getItem(int idx) const;

public slots:
    void deleteAllItems();
    void deleteItems(const QIntList& dbids);
    void addItem(const SQLMangaEntry& file, const Z::Ordering sortOrder, const bool reverseOrder);
};

class ZMangaListView : public QListView {
    Q_OBJECT
protected:
    void resizeEvent(QResizeEvent *e);
private:
    int headerHeight;
public:
    QHeaderView* header;
    ZMangaListView(QWidget *parent = 0);
    ~ZMangaListView();
    void setModel(QAbstractItemModel *model);
    void setViewMode(ViewMode mode);
    void updateHeaderView(const Z::Ordering sortOrder, const bool reverseOrder);
public slots:
    void resizeHeaderView();
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
