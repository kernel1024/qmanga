#include <QApplication>
#include "zdb.h"

ZDB::ZDB(QObject *parent) :
    QObject(parent)
{
    dbBase = QString("qmanga");
    dbUser = QString();
    dbPass = QString();
    wasCanceled = false;
}

int ZDB::getAlbumsCount()
{
    MYSQL* db = sqlOpenBase();
    if (db==NULL) return 0;

    if (mysql_query(db,"SELECT COUNT(name) FROM `albums` ORDER BY name ASC;")) return 0;

    MYSQL_RES* res = mysql_use_result(db);
    int cnt = 0;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != NULL) {
        cnt = QString::fromUtf8(row[0]).toInt();
        break;
    }
    mysql_free_result(res);
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

    MYSQL* db = sqlOpenBase();
    if (db==NULL) return false;

    if (mysql_query(db,"SHOW TABLES;")) return 0;

    MYSQL_RES* res = mysql_use_result(db);

    MYSQL_ROW row;
    int cnt=0;
    while ((row = mysql_fetch_row(res)) != NULL) {
        QString s = QString::fromUtf8(row[0]);
        if (s.contains("files",Qt::CaseInsensitive) ||
                s.contains("albums",Qt::CaseInsensitive))
            cnt++;
    }
    mysql_free_result(res);

    bool noTables = (cnt!=2);

    sqlCloseBase(db);

    if (noTables) {
        emit needTableCreation();
        return false;
    }
    return true;
}

void ZDB::sqlCreateTables()
{
    MYSQL* db = sqlOpenBase();
    if (db==NULL) return;

    QString qr = "CREATE TABLE IF NOT EXISTS `albums` ("
                 "`id` int(11) NOT NULL AUTO_INCREMENT,"
                 "`name` varchar(2048) NOT NULL,"
                 "PRIMARY KEY (`id`)"
                 ") ENGINE=InnoDB DEFAULT CHARSET=utf8;";
    if (mysql_query(db,qr.toUtf8())) {
        emit errorMsg(tr("Unable to create table `albums`\n\n%1")
                      .arg(mysql_error(db)));
        sqlCloseBase(db);
        return;
    }
    qr = "CREATE TABLE IF NOT EXISTS `files` ("
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
    ") ENGINE=InnoDB  DEFAULT CHARSET=utf8;";
    if (mysql_query(db,qr.toUtf8())) {
        emit errorMsg(tr("Unable to create table `files`\n\n%1")
                      .arg(mysql_error(db)));
        sqlCloseBase(db);
        return;
    }
    sqlCloseBase(db);
    return;
}

MYSQL* ZDB::sqlOpenBase()
{
    if (dbUser.isEmpty()) return NULL;
    MYSQL* db = mysql_init(NULL);
    if (!mysql_real_connect(db,"localhost",dbUser.toUtf8(),dbPass.toUtf8(),dbBase.toUtf8(),0,NULL,0)) {
        emit errorMsg(tr("Unable to open MySQL connection. Check login info.\n%1")
                      .arg(mysql_error(db)));
        qDebug() << mysql_error(db);
        db = NULL;
    } else {
        mysql_set_character_set(db,"utf8");
    }
    return db;
}

void ZDB::sqlCloseBase(MYSQL *db)
{
    if (db!=NULL)
        mysql_close(db);
}

void ZDB::sqlGetAlbums()
{
    QStringList result;
    result.clear();
    MYSQL* db = sqlOpenBase();
    if (db==NULL) return;

    QString qr = "SELECT name FROM `albums` ORDER BY name ASC;";
    if (mysql_query(db,qr.toUtf8())) return;

    MYSQL_RES* res = mysql_use_result(db);

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != NULL) {
        result << QString::fromUtf8(row[0]);
    }
    mysql_free_result(res);

    sqlCloseBase(db);
    emit gotAlbums(result);
}

void ZDB::sqlGetFiles(const QString &album, const QString &search, const int sortOrder, const bool reverseOrder)
{
    MYSQL* db = sqlOpenBase();
    if (db==NULL) return;
    Z::Ordering order = (Z::Ordering)sortOrder;

    QTime tmr;
    tmr.start();

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

    int idx = 0;
    if (!mysql_query(db,tqr.toUtf8())) {
        MYSQL_RES* res = mysql_use_result(db);

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != NULL) {
            unsigned long *lengths = mysql_fetch_lengths(res);
            QImage p = QImage();
            QByteArray ba = QByteArray::fromRawData(row[2],lengths[2]);
            if (!ba.isEmpty())
                p.loadFromData(ba);

            emit gotFile(SQLMangaEntry(QString::fromUtf8(row[0]),
                                       QString::fromUtf8(row[1]),
                                       QString::fromUtf8(row[9]),
                                       p,
                                       QString::fromUtf8(row[3]).toInt(),
                                       QString::fromUtf8(row[4]).toInt(),
                                       QString::fromUtf8(row[5]),
                                       QDateTime::fromString(QString::fromUtf8(row[6]),"yyyy-MM-dd H:mm:ss"),
                                       QDateTime::fromString(QString::fromUtf8(row[7]),"yyyy-MM-dd H:mm:ss"),
                                       QString::fromUtf8(row[8]).toInt()));
            QApplication::processEvents();
            idx++;
        }
        mysql_free_result(res);
    } else
        qDebug() << mysql_error(db);
    sqlCloseBase(db);
    emit filesLoaded(idx,tmr.elapsed());
    return;
}

void ZDB::sqlChangeFilePreview(const QString &fileName, const int pageNum)
{
    MYSQL* db = sqlOpenBase();
    if (db==NULL) return;

    QString qrs = QString("SELECT name FROM files WHERE (filename='%1');")
                  .arg(escapeParam(fileName));
    if (mysql_query(db,qrs.toUtf8())) {
        qDebug() << "file search query failed";
        sqlCloseBase(db);
        return;
    }
    MYSQL_RES* res = mysql_use_result(db);

    MYSQL_ROW row = mysql_fetch_row(res);
    if (row==NULL) {
        emit errorMsg("Opened file not found in library.");
        mysql_free_result(res);
        sqlCloseBase(db);
        return;
    }
    mysql_free_result(res);

    QFileInfo fi(fileName);
    if (!fi.isReadable()) {
        qDebug() << "skipping" << fileName << "as unreadable";
        emit errorMsg(tr("%1 file is unreadable.").arg(fileName));
        sqlCloseBase(db);
        return;
    }

    bool mimeOk = false;
    ZAbstractReader* za = readerFactory(this,fileName,&mimeOk,true);
    if (za == NULL) {
        qDebug() << fileName << "File format not supported";
        emit errorMsg(tr("%1 file format not supported.").arg(fileName));
        sqlCloseBase(db);
        return;
    }

    if (!za->openFile()) {
        qDebug() << fileName << "Unable to open file. Broken archive.";
        emit errorMsg(tr("Unable to open file %1. Broken archive.").arg(fileName));
        za->setParent(NULL);
        delete za;
        sqlCloseBase(db);
        return;
    }

    QByteArray pba = createMangaPreview(za,pageNum);
    char* pbae = (char*) malloc(pba.length()*2+1);
    int pbaelen = mysql_real_escape_string(db,pbae,pba.constData(),pba.length());
    pba = QByteArray::fromRawData(pbae,pbaelen);

    QByteArray qr;
    qr.clear();
    qr.append("UPDATE files SET cover='");
    qr.append(pba);
    qr.append(QString("' WHERE (filename='%1');").arg(escapeParam(fileName)));
    if (mysql_real_query(db,qr.constData(),qr.length())) {
        QString msg = tr("Unable to change cover for '%1'.\n%2")
                .arg(fileName)
                .arg(mysql_error(db));
        qDebug() << msg;
        emit errorMsg(msg);
    }
    pba.clear();
    free(pbae);
    za->closeFile();
    za->setParent(NULL);
    delete za;

    sqlCloseBase(db);
    return;
}

void ZDB::sqlRescanIndexedDirs()
{
    MYSQL* db = sqlOpenBase();
    if (db==NULL) return;

    QStringList dirs;
    dirs.clear();

    QString tqr;
    tqr = QString("SELECT filename "
                  "FROM files");
    tqr+=";";

    if (!mysql_query(db,tqr.toUtf8())) {
        MYSQL_RES* res = mysql_use_result(db);

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != NULL) {
            QFileInfo fi(QString::fromUtf8(row[0]));
            if (!fi.exists()) continue;
            if (!dirs.contains(fi.absoluteDir().absolutePath()))
                dirs << fi.absoluteDir().absolutePath();
        }
        mysql_free_result(res);
    } else
        qDebug() << mysql_error(db);
    sqlCloseBase(db);
    if (!dirs.isEmpty())
        emit updateWatchDirList(dirs);
    return;
}

QString mysqlEscapeString(MYSQL* db, const QString& str)
{
    QByteArray pba = str.toUtf8();
    char* pbae = (char*) malloc(pba.length()*2+1);
    int pbaelen = mysql_real_escape_string(db,pbae,pba.constData(),pba.length());
    pba = QByteArray::fromRawData(pbae,pbaelen);
    return QString::fromUtf8(pba);
}

void ZDB::sqlAddFiles(const QStringList& aFiles, const QString& album)
{
    if (album.isEmpty()) {
        emit errorMsg(tr("Album name cannot be empty"));
        return;
    }
    QStringList files = aFiles;
    files.removeDuplicates();

    MYSQL* db = sqlOpenBase();
    if (db==NULL) return;

    int ialbum = -1;
    wasCanceled = false;

    QTime tmr;
    tmr.start();

    QString aqr = QString("SELECT id FROM albums WHERE (name='%1');").arg(escapeParam(album));
    if (!mysql_query(db,aqr.toUtf8())) {
        MYSQL_RES* res = mysql_use_result(db);

        MYSQL_ROW row;
        if ((row = mysql_fetch_row(res)) != NULL)
            ialbum = QString::fromUtf8(row[0]).toInt();
        mysql_free_result(res);
    }

    if (ialbum==-1) {
        QString qr = QString("INSERT INTO albums (name) VALUES ('%1');").arg(album);
        if (mysql_query(db,qr.toUtf8())) {
            emit errorMsg(tr("Unable to create album `%1`\n%2").
                          arg(album).
                          arg(mysql_error(db)));
            sqlCloseBase(db);
            return;
        }
        ialbum = mysql_insert_id(db);
    }

    emit showProgressDialog(true);

    int cnt = 0;
    for (int i=0;i<files.count();i++) {
        QFileInfo fi(files.at(i));
        if (!fi.isReadable()) {
            qDebug() << "skipping" << files.at(i) << "as unreadable";
            continue;
        }

        bool mimeOk = false;
        ZAbstractReader* za = readerFactory(this,files.at(i),&mimeOk,true);
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

        QByteArray pba = createMangaPreview(za,0);
        char* pbae = (char*) malloc(pba.length()*2+1);
        int pbaelen = mysql_real_escape_string(db,pbae,pba.constData(),pba.length());
        pba = QByteArray::fromRawData(pbae,pbaelen);

        QByteArray qr;
        qr.clear();
        qr.append("INSERT INTO files (name, filename, album, cover, pagesCount, fileSize, "
                   "fileMagic, fileDT, addingDT) VALUES ('");
        qr.append(mysqlEscapeString(db,fi.completeBaseName()));
        qr.append("','");
        qr.append(mysqlEscapeString(db,files.at(i)));
        qr.append("','");
        qr.append(QString("%1").arg(ialbum));
        qr.append("','");
        qr.append(pba);
        qr.append("','");
        qr.append(QString("%1").arg(za->getPageCount()));
        qr.append("','");
        qr.append(QString("%1").arg(fi.size()));
        qr.append("','");
        qr.append(za->getMagic());
        qr.append("','");
        qr.append(fi.created().toString("yyyy-MM-dd H:mm:ss"));
        qr.append("',NOW());");
        if (mysql_real_query(db,qr.constData(),qr.length()))
            qDebug() << files.at(i) << "unable to add" << mysql_error(db);
        else
            cnt++;
        pba.clear();
        free(pbae);
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
    sqlRescanIndexedDirs();
    emit filesAdded(cnt,files.count(),tmr.elapsed());
    emit albumsListUpdated();
    return;
}

QByteArray ZDB::createMangaPreview(ZAbstractReader* za, int pageNum)
{
    QByteArray pba;
    pba.clear();

    int idx = pageNum;
    QImage p = QImage();
    QByteArray pb;
    while (idx<za->getPageCount()) {
        p = QImage();
        pb = za->loadPage(idx);
        if (p.loadFromData(pb)) {
            break;
        }
        idx++;
    }
    if (!p.isNull()) {
        p = p.scaled(maxPreviewSize,maxPreviewSize,Qt::KeepAspectRatio,Qt::SmoothTransformation);
        QBuffer buf(&pba);
        buf.open(QIODevice::WriteOnly);
        p.save(&buf, "JPG");
        buf.close();
        p = QImage();
    }
    pb.clear();

    return pba;
}

void ZDB::sqlCancelAdding()
{
    wasCanceled = true;
}

void ZDB::sqlDelFiles(const QIntList &dbids, const bool fullDelete)
{
    if (dbids.count()==0) return;

    QString delqr = QString("DELETE FROM files WHERE id IN (");
    for (int i=0;i<dbids.count();i++) {
        if (i>0) delqr+=",";
        delqr+=QString("%1").arg(dbids.at(i));
    }
    delqr+=");";

    QString fsqr = QString("SELECT filename FROM files WHERE id IN (");
    for (int i=0;i<dbids.count();i++) {
        if (i>0) fsqr+=",";
        fsqr+=QString("%1").arg(dbids.at(i));
    }
    fsqr+=");";

    MYSQL* db = sqlOpenBase();
    bool okdel = true;
    if (db==NULL) return;
    if (fullDelete) {
        if (!mysql_query(db,fsqr.toUtf8())) {
            MYSQL_RES* res = mysql_use_result(db);

            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != NULL) {
                QString fname = QString::fromUtf8(row[0]);
                if (!QFile::remove(fname))
                    okdel = false;
            }
        }
    }

    if (mysql_query(db,delqr.toUtf8())) {
        emit errorMsg(tr("Unable to delete manga\n%2\n%3").
                      arg(mysql_error(db)));
        sqlCloseBase(db);
        return;
    }
    sqlCloseBase(db);
    sqlDelEmptyAlbums();

    emit deleteItemsFromModel(dbids);

    if (!okdel && fullDelete)
        emit errorMsg(tr("Errors occured, some files not deleted from filesystem or not found.\n%2\n%3").
                      arg(mysql_error(db)));
}

void ZDB::sqlDelEmptyAlbums()
{
    QString delqr = QString("DELETE FROM albums WHERE id NOT IN (SELECT DISTINCT album FROM files);");
    MYSQL* db = sqlOpenBase();
    if (db==NULL) return;
    if (mysql_query(db,delqr.toUtf8()))
        qDebug() << "Unable to delete empty albums" << mysql_error(db);
    sqlCloseBase(db);
    emit albumsListUpdated();
}

void ZDB::sqlRenameAlbum(const QString &oldName, const QString &newName)
{
    if (oldName==newName || oldName.isEmpty() || newName.isEmpty()) return;
    QString delqr = QString("UPDATE albums SET name='%1' WHERE name='%2';").
            arg(escapeParam(newName)).arg(escapeParam(oldName));
    MYSQL* db = sqlOpenBase();
    if (db==NULL) return;
    if (mysql_query(db,delqr.toUtf8())) {
        qDebug() << "Unable to rename album" << mysql_error(db);
        emit errorMsg(tr("Unable to rename album `%1`\n%2").
                      arg(oldName).
                      arg(mysql_error(db)));
    }
    sqlCloseBase(db);
    emit albumsListUpdated();
}

void ZDB::sqlDelAlbum(const QString &album)
{
    MYSQL* db = sqlOpenBase();
    if (db==NULL) return;

    QString qra = QString("SELECT id FROM `albums` WHERE name='%1';").arg(escapeParam(album));
    int id = -1;

    if (!mysql_query(db,qra.toUtf8())) {
        MYSQL_RES* res = mysql_use_result(db);
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != NULL) {
            id = QString::fromUtf8(row[0]).toInt();
            break;
        }
        mysql_free_result(res);
    }
    if (id==-1) {
        emit errorMsg(tr("Album '%1' not found").arg(album));
        sqlCloseBase(db);
        return;
    }

    QString qr = QString("DELETE FROM files WHERE album=%1;").arg(id);
    if (mysql_query(db,qr.toUtf8())) {
        QString msg = tr("Unable to delete manga\n%2")
                      .arg(mysql_error(db));
        emit errorMsg(msg);
        qDebug() << msg;
        sqlCloseBase(db);
        return;
    }
    sqlCloseBase(db);
    sqlDelEmptyAlbums();
}
