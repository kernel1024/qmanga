#ifndef ZDB_H
#define ZDB_H

#include <QObject>
#include <QSqlDatabase>
#include "zabstractreader.h"
#include "global.h"

class ZDB : public QObject
{
    Q_OBJECT
protected:
    QString dbBase, dbUser, dbPass;

    bool wasCanceled;

    bool sqlCheckBasePriv();
    QSqlDatabase sqlOpenBase();
    void sqlCloseBase(QSqlDatabase& db);
    QByteArray createMangaPreview(ZAbstractReader *za, int pageNum);

public:
    explicit ZDB(QObject *parent = 0);

    int getAlbumsCount();

signals:
    void errorMsg(const QString& msg);
    void needTableCreation();
    void filesAdded(const int count, const int total, const int elapsed);
    void albumsListUpdated();
    void showProgressDialog(const bool visible);
    void showProgressState(const int value, const QString& msg);
    void gotAlbums(const QStringList& albums);
    void gotFile(const SQLMangaEntry& file);
    void filesLoaded(const int count, const int elapsed);
    void deleteItemsFromModel(const QIntList& dbids);

public slots:
    void setCredentials(const QString& base, const QString& user, const QString& password);
    void sqlCheckBase();
    void sqlCreateTables();
    void sqlDelEmptyAlbums();
    void sqlGetAlbums();
    void sqlRenameAlbum(const QString& oldName, const QString& newName);
    void sqlDelAlbum(const QString& album);
    void sqlDelFiles(const QIntList& dbids);
    void sqlAddFiles(const QStringList& aFiles, const QString& album);
    void sqlCancelAdding();
    void sqlGetFiles(const QString& album, const QString& search, const int sortOrder, const bool reverseOrder);
    void sqlChangeFilePreview(const QString& fileName, const int pageNum);

};

#endif // ZDB_H
