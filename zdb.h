#ifndef ZDB_H
#define ZDB_H

#include <QObject>
#include <QString>
#include <QSqlDatabase>
#include <QThreadPool>
#include <QCache>
#include <QMutex>
#include "readers/zabstractreader.h"
#include "global.h"

class ZDB : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ZDB)
private:
    static inline const int dynamicAlbumParent { -2 };

    bool m_wasCanceled { false };
    QString m_dbHost;
    QString m_dbBase;
    QString m_dbUser;
    QString m_dbPass;
    QStringList m_indexedDirs;
    QStringList m_ignoredFiles;
    ZStrMap m_dynAlbums;
    ZStrHash m_problems;
    QHash<QString,int> m_preferredRendering;
    QThreadPool m_sourceResamplersPool;
    QThreadPool m_searchResamplersPool;
    QThreadPool m_fastResamplersPool;
    QCache<QString, QPair<QSize,QImage> > m_coverCache;
    QMutex m_coverCacheMutex;

    bool sqlCheckBasePriv(QSqlDatabase &db, bool silent);
    bool checkTablesParams(QSqlDatabase &db);
    void checkConfigOpts(QSqlDatabase &db, bool silent);
    QSqlDatabase sqlOpenBase(bool silent = false);
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
    ~ZDB() override = default;

    int getAlbumsCount();
    ZStrMap getDynAlbums() const;
    ZStrHash getConfigProblems() const;
    QStringList sqlGetIgnoredFiles() const;
    Z::PDFRendering getPreferredRendering(const QString& filename) const;
    Z::Ordering getDynAlbumOrdering(const QString &album, Qt::SortOrder &order) const;
    static bool isDynamicAlbumParent(int parentId);
    static int getDynamicAlbumParent();

Q_SIGNALS:
    void errorMsg(const QString& msg);
    void needTableCreation();
    void baseCheckComplete();
    void filesAdded(int count, int total, qint64 elapsed);
    void albumsListUpdated();
    void showProgressDialog(bool visible);
    void showProgressState(int value, const QString& msg);
    void gotAlbums(const ZAlbumVector& albums);
    void gotFile(const ZSQLMangaEntry& file);
    void gotResampledCover(int dbid, const QImage& cover);
    void filesLoaded(int count, qint64 elapsed);
    void deleteItemsFromModel(const ZIntVector& dbids);
    void updateWatchDirList(const QStringList& dirs);
    void foundNewFiles(const QStringList& files);
    void gotTablesDescription(const QString& text);

public Q_SLOTS:
    void setCredentials(const QString &host, const QString& base,
                        const QString& user, const QString& password);
    void setCoverCacheSize(int size);
    void setDynAlbums(const ZStrMap &albums);
    void sqlCheckBase();
    void sqlCreateTables();
    void sqlGetAlbums();
    void sqlRenameAlbum(const QString& oldName, const QString& newName);
    void sqlDelAlbum(const QString& album);
    void sqlAddAlbum(const QString& album, const QString& parent = QString());
    void sqlReparentAlbum(const QString& album, const QString& parent);
    void sqlDelFiles(const ZIntVector& dbids, bool fullDelete);
    void sqlAddFiles(const QStringList& aFiles, const QString& album);
    void sqlCancelAdding();
    void sqlGetFiles(const QString& album, const QString& search, const QSize &preferredCoverSize);
    void sqlChangeFilePreview(const QString& fileName, int pageNum);
    void sqlRescanIndexedDirs();
    void sqlUpdateFileStats(const QString& fileName);
    void sqlSearchMissingManga();
    void sqlAddIgnoredFiles(const QStringList& files);
    void sqlSetIgnoredFiles(const QStringList &files);
    void sqlGetTablesDescription();
    void sqlSetPreferredRendering(const QString& filename, int mode);
};

#endif // ZDB_H
