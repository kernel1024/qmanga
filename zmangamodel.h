#ifndef ZMANGAMODEL_H
#define ZMANGAMODEL_H

#include <QSlider>
#include <QListView>
#include <QAbstractListModel>
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QHeaderView>
#include <QTableView>
#include "global.h"
#include "zglobal.h"
#include "zdb.h"

class ZMangaListView;

class ZMangaModel : public QAbstractTableModel
{
    Q_OBJECT
private:
    QSlider *pixmapSize;
    QTableView *view;

    SQLMangaList mList;

public:
    explicit ZMangaModel(QObject *parent, QSlider *aPixmapSize, QTableView *aView);
    ~ZMangaModel();

    Qt::ItemFlags flags(const QModelIndex & index) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole, bool listMode = false) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    int rowCount( const QModelIndex & parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    int getItemsCount() const;
    SQLMangaEntry getItem(int idx) const;

public slots:
    void deleteAllItems();
    void deleteItems(const QIntList& dbids);
    void addItem(const SQLMangaEntry& file, const Z::Ordering sortOrder, const bool reverseOrder);
};

class ZMangaTableModel : public QSortFilterProxyModel {
    Q_OBJECT
private:
    QTableView *view;

public:
    explicit ZMangaTableModel(QObject *parent, QTableView *aView);
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

};

class ZMangaIconModel : public QSortFilterProxyModel {
    Q_OBJECT
private:
    ZMangaListView *view;

public:
    explicit ZMangaIconModel(QObject *parent, ZMangaListView *aView);
    int columnCount(const QModelIndex &parent) const;

};

class ZMangaListView : public QListView {
    Q_OBJECT
public:
    ZMangaListView(QWidget *parent = nullptr);
protected:
    virtual void updateGeometries();
};

class ZMangaSearchHistoryModel : public QAbstractListModel {
    Q_OBJECT
private:
    QStringList historyItems;
public:
    ZMangaSearchHistoryModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    void setHistoryItems(const QStringList& items);
    QStringList getHistoryItems() const;
    void appendHistoryItem(const QString& item);
};

#endif // ZMANGAMODEL_H
