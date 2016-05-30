#include <QApplication>
#include "zdb.h"

#define FT_DETAILS "1. Edit my.cnf file and save\n" \
                   "--> ft_stopword_file = \"\" (or link an empty file \"empty_stopwords.txt\")\n" \
                   "--> ft_min_word_len = 2\n" \
                   "2. Restart your server (this cannot be done dynamically)\n" \
                   "3. Change the table engine (if needed) - ALTER TABLE tbl_name ENGINE = MyISAM;\n"\
                   "4. Perform repair - REPAIR TABLE tbl_name QUICK;"

ZDB::ZDB(QObject *parent) :
    QObject(parent)
{
    dbBase = QString("qmanga");
    dbUser = QString();
    dbPass = QString();
    wasCanceled = false;
    indexedDirs.clear();
    problems.clear();
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

ZStrMap ZDB::getDynAlbums()
{
    return dynAlbums;
}

QStrHash ZDB::getConfigProblems()
{
    return problems;
}

void ZDB::setCredentials(const QString &base, const QString &user, const QString &password)
{
    dbBase = base;
    dbUser = user;
    dbPass = password;
}

void ZDB::setDynAlbums(const ZStrMap &albums)
{
    dynAlbums.clear();
    dynAlbums = albums;
}

void ZDB::sqlCheckBase()
{
    sqlCheckBasePriv();
    emit baseCheckComplete();
}

bool ZDB::sqlCheckBasePriv()
{
    if (dbUser.isEmpty()) return false;

    QStringList tables;
    tables << "files" << "albums" << "ignored_files";

    MYSQL* db = sqlOpenBase();
    if (db==NULL) return false;

    if (mysql_query(db,"SHOW TABLES;")) return 0;

    MYSQL_RES* res = mysql_use_result(db);

    MYSQL_ROW row;
    int cnt=0;
    while ((row = mysql_fetch_row(res)) != NULL) {
        QString s = QString::fromUtf8(row[0]);
        if (tables.contains(s,Qt::CaseInsensitive))
            cnt++;
    }
    mysql_free_result(res);

    bool noTables = (cnt!=3);

    if (noTables) {
        emit needTableCreation();
        sqlCloseBase(db);
        return false;
    }

    bool ret = checkTablesParams(db);
    sqlCloseBase(db);

    return ret;
}

bool ZDB::checkTablesParams(MYSQL* db)
{
    // Convert tables to MyISAM (if necessary)
    QStringList toconv;
    QString qr = QString("SELECT table_name FROM information_schema.tables "
                         "WHERE (table_schema = DATABASE() AND Engine = \"InnoDB\");");

    if (mysql_query(db,qr.toUtf8())) {
        qDebug() << "Unable to get tables engine." << mysql_error(db);

        problems[tr("MyISAM table engine")] = tr("Unable to check engines for qmanga tables.\n"
                                                 "SELECT query failed.\n"
                                                 "All qmanga tables must be created with MyISAM engine.");

        return false;
    }
    MYSQL_RES* res = mysql_use_result(db);
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != NULL)
        toconv << QString::fromUtf8(row[0]);
    mysql_free_result(res);

    foreach (const QString& tbl, toconv) {
        qr = QString("ALTER TABLE %1 ENGINE=MyISAM;").arg(tbl);
        if (mysql_query(db,qr.toUtf8())) {
            qDebug() << "Engine conversion to MyISAM failed on table" << tbl << mysql_error(db);

            problems[tr("MyISAM table conversion")] = tr("Unable to convert qmanga tables.\n"
                                                         "ALTER TABLE query failed.\n"
                                                         "All qmanga tables must be created with MyISAM engine.");

            return false;
        }
    }

    // Add fulltext index for manga names
    qr = QString("SELECT index_type FROM information_schema.statistics "
                 "WHERE table_schema=DATABASE() AND table_name='files' AND index_name='name_ft';");
    if (mysql_query(db,qr.toUtf8())) {
        qDebug() << "Unable to get indexes list." << mysql_error(db);

        problems[tr("Check for indexes")] = tr("Unable to check indexes for qmanga tables.\n"
                                               "SELECT query failed.\n");

        return false;
    }
    res = mysql_use_result(db);
    QString idxn;
    if ((row = mysql_fetch_row(res)) != NULL)
        idxn = QString::fromUtf8(row[0]);
    mysql_free_result(res);
    if (idxn.isEmpty()) {
        qr = QString("ALTER TABLE files ADD FULLTEXT INDEX name_ft(name);");
        if (mysql_query(db,qr.toUtf8())) {
            qDebug() << "Unable to add fulltext index for files table." << mysql_error(db);

            problems[tr("Creating indexes")] = tr("Unable to add FULLTEXT index for qmanga files table.\n"
                                                  "ALTER TABLE query failed.\n"
                                                  "Fulltext index in necessary for search feature.");

            return false;
        }

        checkConfigOpts(db,false);
    }

    checkConfigOpts(db,true);

    return true;
}

void ZDB::checkConfigOpts(MYSQL *db, bool silent)
{
    if (mysql_query(db,"SELECT @@ft_min_word_len;")) {
        qDebug() << "Unable to check MySQL config options";

        problems[tr("MySQL config")] = tr("Unable to check my.cnf options via global variables.\n"
                                          "SELECT query failed.\n"
                                          "QManga needs specific fulltext ft_* options in config.");

        return;
    }
    MYSQL_RES* res = mysql_use_result(db);
    MYSQL_ROW row;
    if ((row = mysql_fetch_row(res)) != NULL) {
        bool okconv;
        int ftlen = QString::fromUtf8(row[0]).toInt(&okconv);
        if ((ftlen>2 && okconv) || !okconv) {
            if (!silent)
                emit errorMsg(tr(FT_DETAILS));
            problems[tr("MySQL config fulltext")] = tr(FT_DETAILS);
        }
    }
    mysql_free_result(res);
}

void ZDB::sqlCreateTables()
{
    MYSQL* db = sqlOpenBase();
    if (db==NULL) return;

    checkConfigOpts(db,false);

    QString qr = "CREATE TABLE IF NOT EXISTS `albums` ("
                 "`id` int(11) NOT NULL AUTO_INCREMENT,"
                 "`name` varchar(2048) NOT NULL,"
                 "PRIMARY KEY (`id`)"
                 ") ENGINE=MyISAM DEFAULT CHARSET=utf8;";
    if (mysql_query(db,qr.toUtf8())) {
        emit errorMsg(tr("Unable to create table `albums`\n\n%1")
                      .arg(mysql_error(db)));
        problems[tr("Create tables - albums")] = tr("Unable to create table `albums`.\n"
                                                    "CREATE TABLE query failed.");
        sqlCloseBase(db);
        return;
    }
    qr = "CREATE TABLE IF NOT EXISTS `ignored_files` ("
         "`id` int(11) NOT NULL AUTO_INCREMENT,"
         "`filename` varchar(16383) NOT NULL,"
         "PRIMARY KEY (`id`)"
         ") ENGINE=MyISAM DEFAULT CHARSET=utf8;";
    if (mysql_query(db,qr.toUtf8())) {
        emit errorMsg(tr("Unable to create table `ignored_files`\n\n%1")
                      .arg(mysql_error(db)));
        problems[tr("Create tables - ignored_files")] = tr("Unable to create table `ignored_files`.\n"
                                                           "CREATE TABLE query failed.");
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
      "PRIMARY KEY (`id`),"
      "FULLTEXT KEY `name_ft` (`name`)"
    ") ENGINE=MyISAM  DEFAULT CHARSET=utf8;";
    if (mysql_query(db,qr.toUtf8())) {
        emit errorMsg(tr("Unable to create table `files`\n\n%1")
                      .arg(mysql_error(db)));
        problems[tr("Create tables - files")] = tr("Unable to create table `files`.\n"
                                                   "CREATE TABLE query failed.");
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
        problems[tr("Connection")] = tr("Unable to connect to MySQL.\n"
                                        "Check credentials and MySQL running on localhost:3306 socket.");

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
    foreach (const QString& title, dynAlbums.keys())
        result << QString("# %1").arg(title);

    emit gotAlbums(result);
}

void ZDB::sqlGetFiles(const QString &album, const QString &search, const Z::Ordering sortOrder,
                      const bool reverseOrder)
{
    MYSQL* db = sqlOpenBase();
    if (db==NULL) return;

    QTime tmr;
    tmr.start();

    QString tqr = QString("SELECT files.name, filename, cover, pagesCount, fileSize, fileMagic, fileDT, "
                         "addingDT, files.id, albums.name "
                         "FROM files LEFT JOIN albums ON files.album=albums.id ");
    bool specQuery = false;
    if (!album.isEmpty()) {
        if (album.startsWith("# ") && dynAlbums.keys().contains(album.mid(2))) {
            specQuery = true;
            tqr += dynAlbums.value(album.mid(2));
        } else
            tqr += QString("WHERE (album="
                           "  (SELECT id FROM albums WHERE (name = '%1'))"
                           ") ").arg(escapeParam(album));
    } else if (!search.isEmpty())
        tqr += prepareSearchQuery(search);
    if (!specQuery) {
        tqr+="ORDER BY ";
        tqr+=Z::sqlColumns.value(sortOrder);
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
                                       QString::fromUtf8(row[8]).toInt()), sortOrder, reverseOrder);
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

    QString fname(fileName);
    bool dynManga = fname.startsWith("#DYN#");
    if (dynManga)
        fname.remove(QRegExp("^#DYN#"));

    QString qrs = QString("SELECT name FROM files WHERE (filename='%1');")
                  .arg(escapeParam(fname));
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

    QFileInfo fi(fname);
    if (!fi.isReadable()) {
        qDebug() << "skipping" << fname << "as unreadable";
        emit errorMsg(tr("%1 file is unreadable.").arg(fname));
        sqlCloseBase(db);
        return;
    }

    bool mimeOk = false;
    ZAbstractReader* za = readerFactory(this,fileName,&mimeOk,true);
    if (za == NULL) {
        qDebug() << fname << "File format not supported";
        emit errorMsg(tr("%1 file format not supported.").arg(fname));
        sqlCloseBase(db);
        return;
    }

    if (!za->openFile()) {
        qDebug() << fname << "Unable to open file.";
        emit errorMsg(tr("Unable to open file %1.").arg(fname));
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
    qr.append(QString("' WHERE (filename='%1');").arg(escapeParam(fname)));
    if (mysql_real_query(db,qr.constData(),qr.length())) {
        QString msg = tr("Unable to change cover for '%1'.\n%2")
                .arg(fname)
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
                  "FROM files "
                  "WHERE NOT(fileMagic='DYN')");
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
    indexedDirs.clear();
    if (!dirs.isEmpty()) {
        indexedDirs.append(dirs);
        emit updateWatchDirList(dirs);
    }
    emit albumsListUpdated();
    return;
}

void ZDB::sqlUpdateFileStats(const QString &fileName)
{
    bool dynManga = false;
    QString fname = fileName;
    if (fname.startsWith("#DYN#")) {
        fname.remove(QRegExp("^#DYN#"));
        dynManga = true;
    }

    QFileInfo fi(fname);
    if (!fi.isReadable()) {
        qDebug() << "updating aborted for" << fname << "as unreadable";
        return;
    }

    bool mimeOk = false;
    ZAbstractReader* za = readerFactory(this,fileName,&mimeOk,true);
    if (za == NULL) {
        qDebug() << fname << "File format not supported.";
        return;
    }

    if (!za->openFile()) {
        qDebug() << fname << "Unable to open file.";
        za->setParent(NULL);
        delete za;
        return;
    }

    MYSQL* db = sqlOpenBase();
    if (db==NULL) return;

    QByteArray qr;
    qr.clear();
    qr.append("UPDATE files SET ");
    qr.append(QString("pagesCount='%1',").arg(za->getPageCount()));
    if (dynManga)
        qr.append(QString("fileSize='%1',").arg(0));
    else
        qr.append(QString("fileSize='%1',").arg(fi.size()));
    qr.append(QString("fileMagic='%1',").arg(za->getMagic()));
    qr.append(QString("fileDT='%1' ").arg(fi.created().toString("yyyy-MM-dd H:mm:ss")));
    qr.append(QString("WHERE (filename='%1');").arg(escapeParam(fname)));

    za->closeFile();
    za->setParent(NULL);
    delete za;

    if (mysql_real_query(db,qr.constData(),qr.length()))
        qDebug() << fname << "unable to update file stats" << mysql_error(db);

    sqlCloseBase(db);
    return;
}

bool isFileInTable(MYSQL* db, const QString& filename, const QString& table, bool *error)
{
    *error=false;
    QString qr;
    qr = QString("SELECT id FROM %1 WHERE filename='%2';")
         .arg(table)
         .arg(escapeParam(filename));
    if (!mysql_query(db,qr.toUtf8())) {
        MYSQL_RES* res = mysql_store_result(db);
        if (res!=NULL) {
            int rows = mysql_num_rows(res);
            mysql_free_result(res);
            if (rows>0) return true;
        }
    } else {
        *error=true;
    }
    return false;
}

void ZDB::sqlSearchMissingManga()
{
    QStringList foundFiles;
    foundFiles.clear();

    MYSQL* db = sqlOpenBase();
    if (db==NULL) return;

    foreach (const QString& d, indexedDirs) {
        QDir dir(d);
        QFileInfoList fl = dir.entryInfoList(QStringList("*"));
        for (int i=0;i<fl.count();i++)
            if (fl.at(i).isReadable() && fl.at(i).isFile()) {
                bool err1,err2;
                if (!isFileInTable(db,fl.at(i).absoluteFilePath(),"files",&err1) &&
                        !isFileInTable(db,fl.at(i).absoluteFilePath(),"ignored_files",&err2)) {
                    if (!err1 && !err2) {
                        foundFiles << fl.at(i).absoluteFilePath();
                    } else {
                        qDebug() << mysql_error(db);
                        sqlCloseBase(db);
                        return;
                    }
                }
            }
    }
    sqlCloseBase(db);

    emit foundNewFiles(foundFiles);
}

void ZDB::sqlAddIgnoredFiles(const QStringList &files)
{
    MYSQL* db = sqlOpenBase();
    if (db==NULL) return;

    foreach (const QString& file, files) {
        QString qr = QString("INSERT INTO ignored_files (filename) VALUES ('%1');").arg(escapeParam(file));
        if (mysql_query(db,qr.toUtf8())) {
            emit errorMsg(tr("Unable to add ignored file `%1`\n%2").
                          arg(file).
                          arg(mysql_error(db)));
            sqlCloseBase(db);
            return;
        }
    }
    sqlCloseBase(db);
}

void ZDB::sqlGetTablesDescription()
{
    MYSQL* db = sqlOpenBase();
    if (db==NULL) return;

    MYSQL_RES *tbl_cols = mysql_list_fields(db, "files", NULL);
    if (tbl_cols==NULL) return;

    int field_cnt = mysql_num_fields(tbl_cols);

    QStringList names, types;
    int nl = 0; int tl = 0;
    for (int i=0; i < field_cnt; ++i)
    {
        MYSQL_FIELD *col = mysql_fetch_field_direct(tbl_cols, i);
        if (col==NULL) {
            names << "<NULL>";
            types << "unknown";
            continue;
        }
        names << QString::fromUtf8(col->name);
        switch (col->type) {
            case MYSQL_TYPE_DECIMAL:    types << "DECIMAL"; break;
            case MYSQL_TYPE_TINY:       types << "TINY"; break;
            case MYSQL_TYPE_SHORT:      types << "SHORT"; break;
            case MYSQL_TYPE_LONG:       types << "LONG"; break;
            case MYSQL_TYPE_FLOAT:      types << "FLOAT"; break;
            case MYSQL_TYPE_DOUBLE:     types << "DOUBLE"; break;
            case MYSQL_TYPE_NULL:       types << "NULL"; break;
            case MYSQL_TYPE_TIMESTAMP:  types << "TIMESTAMP"; break;
            case MYSQL_TYPE_LONGLONG:   types << "LONGLONG"; break;
            case MYSQL_TYPE_INT24:      types << "INT24"; break;
            case MYSQL_TYPE_DATE:       types << "DATE"; break;
            case MYSQL_TYPE_TIME:       types << "TIME"; break;
            case MYSQL_TYPE_DATETIME:   types << "DATETIME"; break;
            case MYSQL_TYPE_YEAR:       types << "YEAR"; break;
            case MYSQL_TYPE_NEWDATE:    types << "NEWDATE"; break;
            case MYSQL_TYPE_VARCHAR:    types << "VARCHAR"; break;
            case MYSQL_TYPE_BIT:        types << "BIT"; break;
            case MYSQL_TYPE_TIMESTAMP2: types << "TIMESTAMP2"; break;
            case MYSQL_TYPE_DATETIME2:  types << "DATETIME2"; break;
            case MYSQL_TYPE_TIME2:      types << "TIME2"; break;
            case MYSQL_TYPE_NEWDECIMAL: types << "NEWDECIMAL"; break;
            case MYSQL_TYPE_ENUM:       types << "ENUM"; break;
            case MYSQL_TYPE_SET:        types << "SET"; break;
            case MYSQL_TYPE_TINY_BLOB:  types << "TINY_BLOB"; break;
            case MYSQL_TYPE_MEDIUM_BLOB:types << "MEDIUM_BLOB"; break;
            case MYSQL_TYPE_LONG_BLOB:  types << "LONG_BLOB"; break;
            case MYSQL_TYPE_BLOB:       types << "BLOB"; break;
            case MYSQL_TYPE_VAR_STRING: types << "VAR_STRING"; break;
            case MYSQL_TYPE_STRING:     types << "STRING"; break;
            case MYSQL_TYPE_GEOMETRY:   types << "GEOMETRY"; break;
            default:                    types << "unknown"; break;
        }

        if (names.last().length()>nl)
            nl = names.last().length();

        if (types.last().length()>tl)
            tl = types.last().length();
    }
    mysql_free_result(tbl_cols);

    sqlCloseBase(db);

    QStringList res;
    QString s1("Field");
    QString s2("Type");
    res << "Fields description:";
    res << "| "+s1.leftJustified(nl,' ')+" | "+s2.leftJustified(tl,' ')+" |";
    s1.fill('-',nl); s2.fill('-',tl);
    res << "+-"+s1+"-+-"+s2+"-+";
    for (int i=0;i<names.count();i++) {
        s1 = names.at(i);
        s2 = types.at(i);
        res << "| "+s1.leftJustified(nl,' ')+" | "+s2.leftJustified(tl,' ')+" |";
    }
    emit gotTablesDescription(res.join('\n'));
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
    if (album.startsWith("#")) {
        emit errorMsg(tr("Reserved character in album name"));
        return;
    }
    if (aFiles.first().startsWith("###DYNAMIC###")) {
        fsAddImagesDir(aFiles.last(),album);
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
            qDebug() << files.at(i) << "Unable to open file.";
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

void ZDB::fsAddImagesDir(const QString &dir, const QString &album)
{
    QFileInfo fi(dir);
    if (!fi.isReadable()) {
        emit errorMsg(QString("Updating aborted for %1 as unreadable").arg(dir));
        return;
    }

    QDir d(dir);
    QFileInfoList fl = d.entryInfoList(QStringList("*"));
    QStringList files;
    for (int i=0;i<fl.count();i++)
        if (fl.at(i).isReadable() && fl.at(i).isFile() &&
                supportedImg().contains(fl.at(i).suffix(),Qt::CaseInsensitive)) {
            files << fl.at(i).absoluteFilePath();

        }

    if (files.isEmpty()) {
        emit errorMsg("Could not find supported image files in specified directory");
        return;
    }

    MYSQL* db = sqlOpenBase();
    if (db==NULL) return;

    // get album number
    int ialbum = -1;

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

    QTime tmr;
    tmr.start();

    // get album preview image
    QImage p;
    foreach (const QString& fname, files) {
        if (p.load(fname))
            break;
    }

    QByteArray pba;
    char* pbae = NULL;
    pba.clear();
    if (!p.isNull()) {
        p = p.scaled(maxPreviewSize,maxPreviewSize,Qt::KeepAspectRatio,Qt::SmoothTransformation);
        QBuffer buf(&pba);
        buf.open(QIODevice::WriteOnly);
        p.save(&buf, "JPG");
        buf.close();
        p = QImage();

        pbae = (char*) malloc(pba.length()*2+1);
        int pbaelen = mysql_real_escape_string(db,pbae,pba.constData(),pba.length());
        pba = QByteArray::fromRawData(pbae,pbaelen);
    }

    // add dynamic album to base
    int cnt = 0;
    QByteArray qr;
    qr.clear();
    qr.append("INSERT INTO files (name, filename, album, cover, pagesCount, fileSize, "
               "fileMagic, fileDT, addingDT) VALUES ('");
    qr.append(mysqlEscapeString(db,d.dirName()));
    qr.append("','");
    qr.append(mysqlEscapeString(db,d.absolutePath()));
    qr.append("','");
    qr.append(QString("%1").arg(ialbum));
    qr.append("','");
    qr.append(pba);
    qr.append("','");
    qr.append(QString("%1").arg(files.count()));
    qr.append("','");
    qr.append(QString("%1").arg(0));
    qr.append("','");
    qr.append(QString("DYN"));
    qr.append("','");
    qr.append(QString("%1").arg(fi.created().toString("yyyy-MM-dd H:mm:ss")));
    qr.append("',NOW());");
    if (mysql_real_query(db,qr.constData(),qr.length()))
        qDebug() << "unable to add dynamic album" << dir << mysql_error(db);
    else
        cnt++;
    pba.clear();
    if (pbae!=NULL)
        free(pbae);

    sqlCloseBase(db);
    sqlRescanIndexedDirs();
    emit filesAdded(cnt,cnt,tmr.elapsed());
    emit albumsListUpdated();
}

QString ZDB::prepareSearchQuery(const QString &search)
{
    QStringList operators;
    operators << " +" << " ~" << " -" << "* " << "\"";
    QString msearch = search;
    if (search.contains(QRegExp("\\s"))) {
        bool haveops = false;
        for (int i=0;i<operators.count();i++)
            if (msearch.contains(operators.at(i))) {
                haveops = true;
                break;
            }
        if (!haveops) {
            msearch.replace(QRegExp("\\s+"),"* +");
            msearch.prepend(QChar('+'));
            msearch.append(QChar('*'));
        }
        return QString("WHERE MATCH(files.name) AGAINST('%1' IN BOOLEAN MODE) ").arg(escapeParam(msearch));
    } else {
        return QString("WHERE (files.name LIKE '%%%1%%') ").arg(escapeParam(search));
    }
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

    QString fsqr = QString("SELECT filename,fileMagic FROM files WHERE id IN (");
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
                QString magic = QString::fromUtf8(row[1]);
                if (magic!="DYN")
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
    if (newName.startsWith("#") || oldName.startsWith("#")) return;
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
