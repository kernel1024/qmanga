#ifndef ZDB_H
#define ZDB_H

#include <QtCore>
#include <QtSql>
#include "zabstractreader.h"
#include "global.h"

class SQLMangaEntry {
public:
    int dbid;
    QString name;
    QString filename;
    QString album;
    QImage cover;
    int pagesCount;
    qint64 fileSize;
    QString fileMagic;
    QDateTime fileDT;
    QDateTime addingDT;
    SQLMangaEntry();
    SQLMangaEntry(int aDbid);
    SQLMangaEntry(const QString& aName, const QString& aFilename, const QString& aAlbum,
                  const QImage &aCover, const int aPagesCount, const qint64 aFileSize,
                  const QString& aFileMagic, const QDateTime& aFileDT, const QDateTime& aAddingDT, int aDbid);
    SQLMangaEntry &operator=(const SQLMangaEntry& other);
    bool operator==(const SQLMangaEntry& ref) const;
    bool operator!=(const SQLMangaEntry& ref) const;
};

typedef QList<SQLMangaEntry> SQLMangaList;

class ZDB : public QObject
{
    Q_OBJECT
protected:
    QString dbBase, dbUser, dbPass;
    SQLMangaList mList;
    QMutex mLock;
    int mListCount;

    bool wasCanceled;

    bool sqlCheckBasePriv();
    QSqlDatabase sqlOpenBase();
    void sqlCloseBase(QSqlDatabase& db);

public:
    explicit ZDB(QObject *parent = 0);

    // ----- thread-safe -----
    int getResultCount();
    SQLMangaEntry getResultItem(int idx);
    int getAlbumsCount();
    // -----------------------

signals:
    void errorMsg(const QString& msg);
    void needTableCreation();
    void filesAdded(const int count, const int total, const int elapsed);
    void albumsListUpdated();
    void showProgressDialog(const bool visible);
    void showProgressState(const int value, const QString& msg);
    void listRowsAppended();
    void listRowsAboutToAppended(int pos, int last);
    void listRowsDeleted();
    void listRowsAboutToDeleted(int pos, int last);
    void gotAlbums(const QStringList& albums);
    void filesLoaded(const int count, const int elapsed);

public slots:
    void setCredentials(const QString& base, const QString& user, const QString& password);
    void sqlCheckBase();
    void sqlCreateTables();
    void sqlDelEmptyAlbums();
    void sqlGetAlbums();
    void sqlRenameAlbum(const QString& oldName, const QString& newName);
    void sqlDelFiles(const QIntList& dbids);
    void sqlAddFiles(const QStringList& aFiles, const QString& album);
    void sqlCancelAdding();
    void sqlClearList();
    void sqlGetFiles(const QString& album, const QString& search, const int sortOrder, const bool reverseOrder);

};

#endif // ZDB_H
