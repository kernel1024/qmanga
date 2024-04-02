#ifndef ZFILESYSTEMSCANNERMODEL_H
#define ZFILESYSTEMSCANNERMODEL_H

#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QObject>

#include "zsearchtab.h"


class ZFSFile
{
public:
    QString name;
    QString album;
    QString fileName;
    ZFSFile() = default;
    ~ZFSFile() = default;
    ZFSFile(const ZFSFile &other) = default;
    ZFSFile(const QString &aName, const QString &aFileName, const QString &aAlbum);
    ZFSFile &operator=(const ZFSFile &other) = default;
    bool operator==(const ZFSFile &ref) const;
    bool operator!=(const ZFSFile &ref) const;
};

class ZAlbumListDelegate : public QStyledItemDelegate
{
    Q_OBJECT
private:
    QPointer<ZSearchTab> m_searchTab;

public:
    explicit ZAlbumListDelegate(QObject *parent = nullptr);

    void linkWidgets(ZSearchTab *tab);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override;
};

class ZFilesystemScannerModel : public QAbstractTableModel
{
    Q_OBJECT
    Q_DISABLE_COPY(ZFilesystemScannerModel)
private:
    QList<ZFSFile> m_fsScannedFiles;
    QList<ZFSFile> idxListToFiles(const QModelIndexList &rows) const;

public:
    explicit ZFilesystemScannerModel(QObject *parent = nullptr);
    ~ZFilesystemScannerModel() override;

    Qt::ItemFlags flags(const QModelIndex &idx) const override;
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool setData(const QModelIndex &idx, const QVariant &value, int role = Qt::EditRole) override;

    void removeAllFiles();
    void removeFiles(const QModelIndexList &rows);
    QStringList ignoreFiles(const QModelIndexList &rows);
    void addFiles(const QList<ZFSFile> &files);
    void applyAlbum(const QModelIndexList &rows, const QString &album);
    QList<ZFSFile> getFiles() const;

};

#endif // ZFILESYSTEMSCANNERMODEL_H
