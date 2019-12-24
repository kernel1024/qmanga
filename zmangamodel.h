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
#include <QTreeWidget>
#include "global.h"
#include "zglobal.h"
#include "zdb.h"

class ZMangaListView;

class ZMangaModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    ZMangaModel(QObject *parent, QSlider *aPixmapSize, QTableView *aView);
    ~ZMangaModel() override;

    Qt::ItemFlags flags(const QModelIndex & index) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole, bool listMode = false) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    int rowCount( const QModelIndex & parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    int getItemsCount() const;
    ZSQLMangaEntry getItem(int idx) const;

public Q_SLOTS:
    void deleteAllItems();
    void deleteItems(const ZIntVector& dbids);
    void addItem(const ZSQLMangaEntry& file);

private:
    QSlider *m_pixmapSize { nullptr };
    QTableView *m_view { nullptr };
    ZSQLMangaVector m_list;

    Q_DISABLE_COPY(ZMangaModel)
};

class ZMangaTableModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    ZMangaTableModel(QObject *parent, QTableView *aView);
    ~ZMangaTableModel() override = default;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;

private:
    QTableView *m_view { nullptr };

    Q_DISABLE_COPY(ZMangaTableModel)
};

class ZMangaIconModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    ZMangaIconModel(QObject *parent, ZMangaListView *aView);
    ~ZMangaIconModel() override = default;
    int columnCount(const QModelIndex &parent) const override;

private:
    ZMangaListView *m_view { nullptr };

    Q_DISABLE_COPY(ZMangaIconModel)
};

class ZMangaListView : public QListView {
    Q_OBJECT
public:
    explicit ZMangaListView(QWidget *parent = nullptr);
    ~ZMangaListView() override = default;
protected:
    void updateGeometries() override;
private:
    Q_DISABLE_COPY(ZMangaListView)
};

class ZMangaSearchHistoryModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit ZMangaSearchHistoryModel(QObject *parent = nullptr);
    ~ZMangaSearchHistoryModel() override = default;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void setHistoryItems(const QStringList& items);
    QStringList getHistoryItems() const;
    void appendHistoryItem(const QString& item);
private:
    QStringList m_historyItems;

    Q_DISABLE_COPY(ZMangaSearchHistoryModel)
};

class ZAlbumsTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    explicit ZAlbumsTreeWidget(QWidget *parent = nullptr);
    ~ZAlbumsTreeWidget() override = default;
protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent* event) override;
private:
    QModelIndex m_draggingItem;

    Q_DISABLE_COPY(ZAlbumsTreeWidget)
};

#endif // ZMANGAMODEL_H
