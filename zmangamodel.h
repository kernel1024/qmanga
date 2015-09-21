#ifndef ZMANGAMODEL_H
#define ZMANGAMODEL_H

#include <QSlider>
#include <QListView>
#include <QAbstractListModel>
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
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    int rowCount( const QModelIndex & parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent) const;

    void setPixmapSize(QSlider *aPixmapSize);
    int getItemsCount();
    SQLMangaEntry getItem(int idx);

public slots:
    void deleteAllItems();
    void deleteItems(const QIntList& dbids);
    void addItem(const SQLMangaEntry& file);
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
