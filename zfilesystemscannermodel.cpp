#include <QComboBox>
#include <QGuiApplication>

#include "zfilesystemscannermodel.h"

namespace ZDefaults {
const int fsColumnsCount = 2;
}

ZFilesystemScannerModel::ZFilesystemScannerModel(QObject *parent)
    : QAbstractTableModel{parent}
{}

ZFilesystemScannerModel::~ZFilesystemScannerModel()
{
    m_fsScannedFiles.clear();
}

Qt::ItemFlags ZFilesystemScannerModel::flags(const QModelIndex &idx) const
{
    if (!checkIndex(idx, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
        return Qt::NoItemFlags;

    Qt::ItemFlags res = (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    if (idx.column() == 1)
        res |= Qt::ItemIsEditable;

    return res;
}

QVariant ZFilesystemScannerModel::data(const QModelIndex &idx, int role) const
{
    if (!checkIndex(idx, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
        return QVariant();

    const int row = idx.row();
    const int col = idx.column();
    const ZFSFile t = m_fsScannedFiles.at(row);

    if (role == Qt::DisplayRole) {
        switch (col) {
            case 0:
                return t.name;
            case 1:
                return t.album;
            default:
                return QVariant();
        }
    }

    if (role == Qt::ToolTipRole || role == Qt::StatusTipRole) {
        return t.fileName;
    }

    return QVariant();
}

int ZFilesystemScannerModel::rowCount(const QModelIndex &parent) const
{
    if (!checkIndex(parent))
        return 0;
    if (parent.isValid())
        return 0;

    return m_fsScannedFiles.count();
}

int ZFilesystemScannerModel::columnCount(const QModelIndex &parent) const
{
    if (!checkIndex(parent))
        return 0;
    if (parent.isValid())
        return 0;

    return ZDefaults::fsColumnsCount;
}

QVariant ZFilesystemScannerModel::headerData(int section,
                                             Qt::Orientation orientation,
                                             int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0:
                return tr("File");
            case 1:
                return tr("Album");
            default:
                return QVariant();
        }
    }

    return QVariant();
}

bool ZFilesystemScannerModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    Q_UNUSED(role)

    if (!checkIndex(idx, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
        return false;

    const int row = idx.row();
    const QString album = value.toString();

    if ((row < 0) || (row >= m_fsScannedFiles.count()) || (album.isEmpty())) {
        return false;
    }

    m_fsScannedFiles[row].album = album;

    Q_EMIT dataChanged(index(row, 0), index(row, 1));

    return true;
}

QList<ZFSFile> ZFilesystemScannerModel::idxListToFiles(const QModelIndexList &rows) const
{
    QList<ZFSFile> list;
    for (const auto &i : rows) {
        const auto item = m_fsScannedFiles.at(i.row());
        if (!list.contains(item))
            list.append(item);
    }
    return list;
}

void ZFilesystemScannerModel::removeAllFiles()
{
    if (m_fsScannedFiles.isEmpty())
        return;
    beginRemoveRows(QModelIndex(), 0, m_fsScannedFiles.count() - 1);
    m_fsScannedFiles.clear();
    endRemoveRows();
}

void ZFilesystemScannerModel::removeFiles(const QModelIndexList &rows)
{
    for (const auto &item : idxListToFiles(rows)) {
        int const idx = m_fsScannedFiles.indexOf(item);
        if (idx >= 0) {
            beginRemoveRows(QModelIndex(), idx, idx);
            m_fsScannedFiles.removeAt(idx);
            endRemoveRows();
        }
    }
}

QStringList ZFilesystemScannerModel::ignoreFiles(const QModelIndexList &rows)
{
    QStringList res; // filenames to ignore for ZDB
    for (const auto &item : idxListToFiles(rows))
        res.append(item.fileName);

    removeFiles(rows);
    return res;
}

void ZFilesystemScannerModel::addFiles(const QList<ZFSFile> &files)
{
    QList<ZFSFile> list;
    for (const auto &item : files) {
        if (m_fsScannedFiles.contains(item))
            list.append(item);
    }
    if (list.isEmpty())
        return;

    int const posidx = list.count();
    beginInsertRows(QModelIndex(), posidx, posidx + list.count() - 1);
    m_fsScannedFiles.append(list);
    endInsertRows();
}

QList<ZFSFile> ZFilesystemScannerModel::getFiles() const
{
    return m_fsScannedFiles;
}

ZFSFile::ZFSFile(const QString &aName, const QString &aFileName, const QString &aAlbum)
    : name(aName)
    , album(aAlbum)
    , fileName(aFileName)
{
}

bool ZFSFile::operator==(const ZFSFile &ref) const
{
    return ((name == ref.name) && (fileName == ref.fileName) && (album == ref.album));
}

bool ZFSFile::operator!=(const ZFSFile &ref) const
{
    return !operator==(ref);
}

ZAlbumListDelegate::ZAlbumListDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

void ZAlbumListDelegate::linkWidgets(ZSearchTab *tab)
{
    m_searchTab = tab;
}

QWidget *ZAlbumListDelegate::createEditor(QWidget *parent,
                                          const QStyleOptionViewItem &option,
                                          const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    auto *editor = new QComboBox(parent);
    editor->addItems(m_searchTab->getAlbums());
    editor->setEditable(true);

    return editor;
}

void ZAlbumListDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    const QString album = index.model()->data(index, Qt::EditRole).toString();

    auto *list = static_cast<QComboBox*>(editor);
    list->setCurrentText(album);
}

void ZAlbumListDelegate::setModelData(QWidget *editor,
                                      QAbstractItemModel *model,
                                      const QModelIndex &index) const
{
    auto *list = static_cast<QComboBox *>(editor);
    model->setData(index, list->currentText(), Qt::EditRole);
}

void ZAlbumListDelegate::updateEditorGeometry(QWidget *editor,
                                              const QStyleOptionViewItem &option,
                                              const QModelIndex &index) const
{
    Q_UNUSED(index)

    editor->setGeometry(option.rect);
}
