#ifndef ZDB_H
#define ZDB_H

#include <QObject>
#include <QString>
#include <QSqlDatabase>
#include "zabstractreader.h"
#include "global.h"

const int dynamicAlbumParent = -2;

class ZDB : public QObject
{
    Q_OBJECT
private:
    QString dbHost, dbBase, dbUser, dbPass;
    QStringList indexedDirs, ignoredFiles;
    ZStrMap dynAlbums;
    QStrHash problems;
    QHash<QString,int> preferredRendering;

    bool wasCanceled;

    bool sqlCheckBasePriv(QSqlDatabase &db, bool silent);
    bool checkTablesParams(QSqlDatabase &db);
    void checkConfigOpts(QSqlDatabase &db, bool silent);
    QSqlDatabase sqlOpenBase();
    void sqlCloseBase(QSqlDatabase &db);
    void sqlUpdateIgnoredFiles(QSqlDatabase &db);
    QByteArray createMangaPreview(ZAbstractReader *za, int pageNum);
    void fsAddImagesDir(const QString& dir, const QString& album);
    QString prepareSearchQuery(const QString& search);
    void sqlInsertIgnoredFilesPrivate(const QStringList &files, bool cleanTable);
    Z::DBMS sqlDbEngine(QSqlDatabase &db);
    bool sqlHaveTables(QSqlDatabase &db);
    int sqlFindAndAddAlbum(const QString& name, const QString& parent = QString(),
                           bool createNew = true);

public:
    explicit ZDB(QObject *parent = nullptr);
    ~ZDB() = default;

    int getAlbumsCount();
    ZStrMap getDynAlbums() const;
    QStrHash getConfigProblems() const;
    QStringList sqlGetIgnoredFiles() const;
    Z::PDFRendering getPreferredRendering(const QString& filename) const;
    Z::Ordering getDynAlbumOrdering(const QString &album, Qt::SortOrder &order) const;

signals:
    void errorMsg(const QString& msg);
    void needTableCreation();
    void baseCheckComplete();
    void filesAdded(const int count, const int total, const int elapsed);
    void albumsListUpdated();
    void showProgressDialog(const bool visible);
    void showProgressState(const int value, const QString& msg);
    void gotAlbums(const AlbumVector& albums);
    void gotFile(const SQLMangaEntry& file);
    void filesLoaded(const int count, const int elapsed);
    void deleteItemsFromModel(const QIntVector& dbids);
    void updateWatchDirList(const QStringList& dirs);
    void foundNewFiles(const QStringList& files);
    void gotTablesDescription(const QString& text);

public slots:
    void setCredentials(const QString &host, const QString& base,
                        const QString& user, const QString& password);
    void setDynAlbums(const ZStrMap &albums);
    void sqlCheckBase();
    void sqlCreateTables();
    void sqlGetAlbums();
    void sqlRenameAlbum(const QString& oldName, const QString& newName);
    void sqlDelAlbum(const QString& album);
    void sqlAddAlbum(const QString& album, const QString& parent = QString());
    void sqlReparentAlbum(const QString& album, const QString& parent);
    void sqlDelFiles(const QIntVector& dbids, const bool fullDelete);
    void sqlAddFiles(const QStringList& aFiles, const QString& album);
    void sqlCancelAdding();
    void sqlGetFiles(const QString& album, const QString& search);
    void sqlChangeFilePreview(const QString& fileName, const int pageNum);
    void sqlRescanIndexedDirs();
    void sqlUpdateFileStats(const QString& fileName);
    void sqlSearchMissingManga();
    void sqlAddIgnoredFiles(const QStringList& files);
    void sqlSetIgnoredFiles(const QStringList &files);
    void sqlGetTablesDescription();
    bool sqlSetPreferredRendering(const QString& filename, int mode);
};

#endif // ZDB_H
