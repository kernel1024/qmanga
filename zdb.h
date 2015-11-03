#ifndef ZDB_H
#define ZDB_H

#include <QObject>
#include <QString>
#include "zabstractreader.h"
#include "global.h"

namespace mysql {
    #include <mysql/mysql.h>
}

using namespace mysql;

class ZDB : public QObject
{
    Q_OBJECT
protected:
    QString dbBase, dbUser, dbPass;
    QStringList indexedDirs;
    ZStrMap dynAlbums;

    bool wasCanceled;

    bool sqlCheckBasePriv();
    MYSQL* sqlOpenBase();
    void sqlCloseBase(MYSQL* db);
    QByteArray createMangaPreview(ZAbstractReader *za, int pageNum);
    void fsAddImagesDir(const QString& dir, const QString& album);

public:
    explicit ZDB(QObject *parent = 0);

    int getAlbumsCount();
    ZStrMap getDynAlbums();

signals:
    void errorMsg(const QString& msg);
    void needTableCreation();
    void filesAdded(const int count, const int total, const int elapsed);
    void albumsListUpdated();
    void showProgressDialog(const bool visible);
    void showProgressState(const int value, const QString& msg);
    void gotAlbums(const QStringList& albums);
    void gotFile(const SQLMangaEntry& file, const Z::Ordering sortOrder, const bool reverseOrder);
    void filesLoaded(const int count, const int elapsed);
    void deleteItemsFromModel(const QIntList& dbids);
    void updateWatchDirList(const QStringList& dirs);
    void foundNewFiles(const QStringList& files);
    void gotTablesDescription(const QString& text);

public slots:
    void setCredentials(const QString& base, const QString& user, const QString& password);
    void setDynAlbums(const ZStrMap &albums);
    void sqlCheckBase();
    void sqlCreateTables();
    void sqlDelEmptyAlbums();
    void sqlGetAlbums();
    void sqlRenameAlbum(const QString& oldName, const QString& newName);
    void sqlDelAlbum(const QString& album);
    void sqlDelFiles(const QIntList& dbids, const bool fullDelete);
    void sqlAddFiles(const QStringList& aFiles, const QString& album);
    void sqlCancelAdding();
    void sqlGetFiles(const QString& album, const QString& search, const Z::Ordering sortOrder, const bool reverseOrder);
    void sqlChangeFilePreview(const QString& fileName, const int pageNum);
    void sqlRescanIndexedDirs();
    void sqlUpdateFileStats(const QString& fileName);
    void sqlSearchMissingManga();
    void sqlAddIgnoredFiles(const QStringList& files);
    void sqlGetTablesDescription();

};

#endif // ZDB_H
