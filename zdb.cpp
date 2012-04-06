#include "zdb.h"

ZDB::ZDB(QObject *parent) :
    QObject(parent)
{
    dbBase = QString("qmanga");
    dbUser = QString();
    dbPass = QString();
    wasCanceled = false;
    mListCount = 0;
    mList.clear();
}

int ZDB::getResultCount()
{
    return mListCount;
}

SQLMangaEntry ZDB::getResultItem(int idx)
{
    mLock.lock();
    SQLMangaEntry m = SQLMangaEntry();
    if (idx>=0 && idx<mList.count())
        m = mList.at(idx);
    mLock.unlock();
    return m;
}

int ZDB::getAlbumsCount()
{
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return 0;

    QSqlQuery qr("SELECT COUNT(name) FROM `albums` ORDER BY name ASC",db);
    int cnt = 0;
    while (qr.next()) {
        cnt = qr.value(0).toInt();
        break;
    }
    sqlCloseBase(db);
    return cnt;
}

void ZDB::setCredentials(const QString &base, const QString &user, const QString &password)
{
    dbBase = base;
    dbUser = user;
    dbPass = password;
}

void ZDB::sqlCheckBase()
{
    sqlCheckBasePriv();
}

bool ZDB::sqlCheckBasePriv()
{
    if (dbUser.isEmpty()) return false;

    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) {
        emit errorMsg(tr("Unable to open MySQL connection. Check login info."));
        return false;
    }
    bool noTables = (!db.tables().contains("files",Qt::CaseInsensitive) ||
            !db.tables().contains("albums",Qt::CaseInsensitive));
    sqlCloseBase(db);
    if (noTables) {
        emit needTableCreation();
        return false;
    }
    return true;
}

void ZDB::sqlCreateTables()
{
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    QSqlQuery qr(db);
    if (!qr.exec("CREATE TABLE IF NOT EXISTS `albums` ("
                 "`id` int(11) NOT NULL AUTO_INCREMENT,"
                 "`name` varchar(2048) NOT NULL,"
                 "PRIMARY KEY (`id`)"
                 ") ENGINE=InnoDB DEFAULT CHARSET=utf8;")) {
        sqlCloseBase(db);
        emit errorMsg(tr("Unable to create table `albums`\n\n%1\n%2").
                      arg(qr.lastError().databaseText()).arg(qr.lastError().driverText()));
        return;
    }
    if (!qr.exec("CREATE TABLE IF NOT EXISTS `files` ("
      "`id` int(11) NOT NULL AUTO_INCREMENT,"
      "`name` varchar(2048) NOT NULL,"
      "`filename` varchar(16383) NOT NULL,"
      "`album` int(11) NOT NULL,"
      "`cover` mediumblob,"
      "`pagesCount` int(11) NOT NULL,"
      "`fileSize` bigint(20) NOT NULL,"
      "`fileMagic` varchar(32) NOT NULL,"
      "`fileDT` datetime NOT NULL,"
      "`addingDT` datetime NOT NULL,"
      "PRIMARY KEY (`id`)"
    ") ENGINE=InnoDB  DEFAULT CHARSET=utf8")) {
        sqlCloseBase(db);
        emit errorMsg(tr("Unable to create table `albums`\n\n%1\n%2").
                      arg(qr.lastError().databaseText()).arg(qr.lastError().driverText()));
        return;
    }
    sqlCloseBase(db);
    return;
}

QSqlDatabase ZDB::sqlOpenBase()
{
    if (dbUser.isEmpty()) return QSqlDatabase();
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL",QUuid::createUuid().toString());
    if (db.isValid()) {
        db.setHostName("localhost");
        db.setDatabaseName(dbBase);
        db.setUserName(dbUser);
        db.setPassword(dbPass);
        if (!db.open())
            db = QSqlDatabase();
    }
    return db;
}

void ZDB::sqlCloseBase(QSqlDatabase &db)
{
    if (db.isOpen())
        db.close();
}

void ZDB::sqlGetAlbums()
{
    QStringList res;
    res.clear();
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    QSqlQuery qr("SELECT name FROM `albums` ORDER BY name ASC",db);
    while (qr.next()) {
        res << qr.value(0).toString();
    }
    sqlCloseBase(db);
    emit gotAlbums(res);
}

void ZDB::sqlGetFiles(const QString &album, const QString &search, Z::Ordering order, bool reverseOrder)

{
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    listRowsAboutToDeleted(0,mListCount-1);
    mLock.lock();
    mList.clear();
    mListCount = 0;
    mLock.unlock();
    listRowsDeleted();

    QTime tmr;
    tmr.start();

    QSqlQuery qr(db);
    QString tqr;
    if (!album.isEmpty()) {
        tqr = QString("SELECT files.name, filename, cover, pagesCount, fileSize, fileMagic, fileDT, "
                      "addingDT, files.id, albums.name "
                      "FROM files LEFT JOIN albums ON files.album=albums.id "
                      "WHERE (album="
                      "  (SELECT id FROM albums WHERE (name = '%1'))"
                      ") ").arg(escapeParam(album));
    } else if (!search.isEmpty()){
        tqr = QString("SELECT files.name, filename, cover, pagesCount, fileSize, fileMagic, fileDT, "
                "addingDT, files.id, albums.name "
                "FROM files LEFT JOIN albums ON files.album=albums.id "
                "WHERE (files.name LIKE '%%%1%%') ").arg(escapeParam(search));
    } else {
        tqr = QString("SELECT files.name, filename, cover, pagesCount, fileSize, fileMagic, fileDT, "
                "addingDT, files.id, albums.name "
                "FROM files LEFT JOIN albums ON files.album=albums.id ");
    }
    if (order!=Z::Unordered) {
        tqr+="ORDER BY ";
        if (order==Z::FileDate) tqr+="fileDT";
        else if (order==Z::AddingDate) tqr+="addingDT";
        else if (order==Z::Album) tqr+="albums.name";
        else if (order==Z::PagesCount) tqr+="pagesCount";
        else tqr+="files.name";
        if (reverseOrder) tqr+=" DESC";
        else tqr+=" ASC";
    }
    tqr+=";";
    qr.prepare(tqr);

    int idx = 0;
    if (qr.exec()) {
        //int resCnt = qr.size();
        while (qr.next()) {
            QImage p = QImage();
            QByteArray ba = qr.value(2).toByteArray();
            if (!ba.isEmpty())
                p.loadFromData(ba);

            mLock.lock();
            int posidx = mList.count();
            mLock.unlock();
            listRowsAboutToAppended(posidx,posidx+1);
            mLock.lock();
            mList << SQLMangaEntry(qr.value(0).toString(),
                                   qr.value(1).toString(),
                                   qr.value(9).toString(),
                                   p,
                                   qr.value(3).toInt(),
                                   qr.value(4).toInt(),
                                   qr.value(5).toString(),
                                   qr.value(6).toDateTime(),
                                   qr.value(7).toDateTime(),
                                   qr.value(8).toInt());
            mListCount++;
            mLock.unlock();
            listRowsAppended();
            idx++;
        }
    } else
        qDebug() << qr.lastError().databaseText() << qr.lastError().driverText();
    sqlCloseBase(db);
    emit filesLoaded(idx,tmr.elapsed());
    return;
}

void ZDB::sqlAddFiles(const QStringList& aFiles, const QString& album)
{
    if (album.isEmpty()) {
        emit errorMsg(tr("Album name cannot be empty"));
        return;
    }
    QStringList files = aFiles;
    files.removeDuplicates();

    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    int ialbum = -1;
    wasCanceled = false;

    QTime tmr;
    tmr.start();

    QSqlQuery aqr(db);
    aqr.prepare(QString("SELECT id FROM albums WHERE (name='%1');").arg(escapeParam(album)));
    if (aqr.exec())
        if (aqr.next())
            ialbum = aqr.value(0).toInt();

    if (ialbum==-1) {
        QSqlQuery qr(db);
        qr.prepare("INSERT INTO albums (name) VALUES (?);");
        qr.bindValue(0,album);
        if (!qr.exec()) {
            emit errorMsg(tr("Unable to create album `%1`\n%2\n%3").
                          arg(album).
                          arg(qr.lastError().databaseText()).
                          arg(qr.lastError().driverText()));
            sqlCloseBase(db);
            return;
        }
        ialbum = qr.lastInsertId().toInt();
    }

    emit showProgressDialog(true);

    int cnt = 0;
    for (int i=0;i<files.count();i++) {
        QFileInfo fi(files.at(i));
        if (!fi.isReadable()) {
            qDebug() << "skipping" << files.at(i) << "as unreadable";
            continue;
        }

        ZAbstractReader* za = readerFactory(this,files.at(i));
        if (za == NULL) {
            qDebug() << files.at(i) << "File format not supported.";
            continue;
        }
        if (!za->openFile()) {
            qDebug() << files.at(i) << "Unable to open file. Broken archive.";
            za->setParent(NULL);
            delete za;
            continue;
        }
        int idx = 0;
        QImage p = QImage();
        while (idx<za->getPageCount()) {
            QImage p = za->loadPage(idx);
            if (!p.isNull()) {
                break;
            }
            idx++;
        }
        QByteArray pba;
        if (!p.isNull()) {
            p = p.scaled(maxPreviewSize,maxPreviewSize,Qt::KeepAspectRatio,Qt::SmoothTransformation);
            QBuffer buf(&pba);
            buf.open(QIODevice::WriteOnly);
            p.save(&buf, "JPG");
            buf.close();
        }

        QSqlQuery qr(db);
        qr.prepare("INSERT INTO files (name, filename, album, cover, pagesCount, fileSize, "
                   "fileMagic, fileDT, addingDT) VALUES (?, ?, ?, ?, ?, ?, ?, ?, NOW());");
        qr.bindValue(0,fi.completeBaseName());
        qr.bindValue(1,files.at(i));
        qr.bindValue(2,ialbum);
        qr.bindValue(3,pba);
        qr.bindValue(4,za->getPageCount());
        qr.bindValue(5,fi.size());
        qr.bindValue(6,za->getMagic());
        qr.bindValue(7,fi.created());
        if (!qr.exec())
            qDebug() << files.at(i) << "unable to add" << qr.lastError().databaseText() <<
                        qr.lastError().driverText();
        else
            cnt++;
        pba.clear();
        p = QImage();
        za->closeFile();
        za->setParent(NULL);
        delete za;

        QString s = fi.fileName();
        if (s.length()>60) s = s.left(60)+"...";
        emit showProgressState(100*i/files.count(),s);
        QApplication::processEvents();

        if (wasCanceled) {
            wasCanceled = false;
            break;
        }
        QApplication::processEvents();
    }
    emit showProgressDialog(false);
    sqlCloseBase(db);
    emit filesAdded(cnt,files.count(),tmr.elapsed());
    emit albumsListUpdated();
    return;
}

void ZDB::sqlCancelAdding()
{
    wasCanceled = true;
}

void ZDB::sqlClearList()
{
    emit listRowsAboutToDeleted(0,mListCount);
    mLock.lock();
    mList.clear();
    mListCount = 0;
    mLock.unlock();
    emit listRowsDeleted();
}

void ZDB::sqlDelFiles(const QIntList &dbids)
{
    if (dbids.count()==0) return;

    QString delqr = QString("DELETE FROM files WHERE id IN (");
    for (int i=0;i<dbids.count();i++) {
        if (i>0) delqr+=",";
        delqr+=QString("%1").arg(dbids.at(i));
    }
    delqr+=");";

    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;
    QSqlQuery qr(db);
    qr.prepare(delqr);
    if (!qr.exec()) {
        emit errorMsg(tr("Unable to delete manga\n%2\n%3").
                      arg(qr.lastError().databaseText()).
                      arg(qr.lastError().driverText()));
        sqlCloseBase(db);
        return;
    }
    sqlCloseBase(db);
    sqlDelEmptyAlbums();

    for (int i=0;i<dbids.count();i++) {
        mLock.lock();
        int idx = mList.indexOf(SQLMangaEntry(dbids.at(i)));
        mLock.unlock();
        if (idx>=0) {
            emit listRowsAboutToDeleted(idx,idx+1);
            mLock.lock();
            mList.removeAt(idx);
            mLock.unlock();
            emit listRowsDeleted();
        }
    }
}

void ZDB::sqlDelEmptyAlbums()
{
    QString delqr = QString("DELETE FROM albums WHERE id NOT IN (SELECT DISTINCT album FROM files);");
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;
    QSqlQuery qr(db);
    qr.prepare(delqr);
    if (!qr.exec())
        qDebug() << "Unable to delete empty albums" << qr.lastError().databaseText() <<
                    qr.lastError().driverText();
    sqlCloseBase(db);
    emit albumsListUpdated();
}

void ZDB::sqlRenameAlbum(const QString &oldName, const QString &newName)
{
    if (oldName==newName || oldName.isEmpty() || newName.isEmpty()) return;
    QString delqr = QString("UPDATE albums SET name='%1' WHERE name='%2';").
            arg(escapeParam(newName)).arg(escapeParam(oldName));
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;
    QSqlQuery qr(db);
    qr.prepare(delqr);
    if (!qr.exec()) {
        qDebug() << "Unable to rename album" <<
                    qr.lastError().databaseText() <<
                    qr.lastError().driverText();
        emit errorMsg(tr("Unable to rename album `%1`\n%2\n%3").
                      arg(oldName).
                      arg(qr.lastError().databaseText()).
                      arg(qr.lastError().driverText()));
    }
    sqlCloseBase(db);
    emit albumsListUpdated();
}

SQLMangaEntry::SQLMangaEntry()
{
    name = QString();
    filename = QString();
    cover = QImage();
    album = QString();
    pagesCount = -1;
    fileSize = -1;
    fileMagic = QString();
    fileDT = QDateTime();
    addingDT = QDateTime();
    dbid = -1;
}

SQLMangaEntry::SQLMangaEntry(int aDbid)
{
    name = QString();
    filename = QString();
    cover = QImage();
    album = QString();
    pagesCount = -1;
    fileSize = -1;
    fileMagic = QString();
    fileDT = QDateTime();
    addingDT = QDateTime();
    dbid = aDbid;
}

SQLMangaEntry::SQLMangaEntry(const QString &aName, const QString &aFilename, const QString &aAlbum,
                             const QImage &aCover, const int aPagesCount, const qint64 aFileSize,
                             const QString &aFileMagic, const QDateTime &aFileDT,
                             const QDateTime &aAddingDT, int aDbid)
{
    name = aName;
    filename = aFilename;
    album = aAlbum;
    cover = aCover;
    pagesCount = aPagesCount;
    fileSize = aFileSize;
    fileMagic = aFileMagic;
    fileDT = aFileDT;
    addingDT = aAddingDT;
    dbid = aDbid;
}

SQLMangaEntry &SQLMangaEntry::operator =(const SQLMangaEntry &other)
{
    name = other.name;
    filename = other.filename;
    album = other.album;
    cover = other.cover;
    pagesCount = other.pagesCount;
    fileSize = other.fileSize;
    fileMagic = other.fileMagic;
    fileDT = other.fileDT;
    addingDT = other.addingDT;
    dbid = other.dbid;
    return *this;
}

bool SQLMangaEntry::operator ==(const SQLMangaEntry &ref) const
{
    return (ref.dbid==dbid);
}

bool SQLMangaEntry::operator !=(const SQLMangaEntry &ref) const
{
    return (ref.dbid!=dbid);
}
