#include <QApplication>
#include <QTemporaryFile>
#include <QTextStream>
#include <QBuffer>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDriver>
#include <QSqlRecord>
#include <QSqlField>
#include <QVariant>
#include <QDebug>
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
    dbHost = QString("localhost");
    dbBase = QString("qmanga");
    dbUser = QString();
    dbPass = QString();
    wasCanceled = false;
    indexedDirs.clear();
    problems.clear();
    ignoredFiles.clear();
    preferredRendering.clear();
}

int ZDB::getAlbumsCount()
{
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return 0;

    QSqlQuery qr("SELECT COUNT(name) FROM `albums` ORDER BY name ASC",db);
    int cnt = 0;
    while (qr.next())
        cnt = qr.value(0).toInt();

    sqlCloseBase(db);
    return cnt;
}

ZStrMap ZDB::getDynAlbums() const
{
    return dynAlbums;
}

QStrHash ZDB::getConfigProblems() const
{
    return problems;
}

void ZDB::setCredentials(const QString &host, const QString &base, const QString &user, const QString &password)
{
    dbHost = host;
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

    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return false;

    int cnt=0;
    foreach (const QString& s, db.tables(QSql::Tables)) {
        if (tables.contains(s,Qt::CaseInsensitive))
            cnt++;
    }

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

bool ZDB::checkTablesParams(QSqlDatabase &db)
{
    if (!isMySQL(db)) return true;

    QSqlQuery qr(db);

    // Check for preferred rendering column
    qr.exec("SHOW COLUMNS FROM files");
    QStringList cols;
    while (qr.next())
        cols << qr.value(0).toString();

    if (!cols.contains("preferredRendering",Qt::CaseInsensitive)) {
        if (!qr.exec("ALTER TABLE `files` ADD COLUMN `preferredRendering` INT(11) DEFAULT 0")) {
            qDebug() << "Unable to add column for files table."
                     << qr.lastError().databaseText() << qr.lastError().driverText();

            problems[tr("Adding column")] = tr("Unable to add column for qmanga files table.\n"
                                               "ALTER TABLE query failed.");

            return false;
        }
    }

    // Add fulltext index for manga names
    if (!qr.exec("SELECT index_type FROM information_schema.statistics "
                 "WHERE table_schema=DATABASE() AND table_name='files' AND index_name='name_ft'")) {
        qDebug() << "Unable to get indexes list."
                 << qr.lastError().databaseText() << qr.lastError().driverText();

        problems[tr("Check for indexes")] = tr("Unable to check indexes for qmanga tables.\n"
                                               "SELECT query failed.\n");

        return false;
    }
    QString idxn;
    if (qr.next())
        idxn = qr.value(0).toString();
    if (idxn.isEmpty()) {
        if (!qr.exec("ALTER TABLE files ADD FULLTEXT INDEX name_ft(name)")) {
            qDebug() << "Unable to add fulltext index for files table."
                     << qr.lastError().databaseText() << qr.lastError().driverText();

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

void ZDB::checkConfigOpts(QSqlDatabase &db, bool silent)
{
    QSqlQuery qr(db);
    if (!qr.exec("SELECT @@ft_min_word_len")) {
        qDebug() << "Unable to check MySQL config options";

        problems[tr("MySQL config")] = tr("Unable to check my.cnf options via global variables.\n"
                                          "SELECT query failed.\n"
                                          "QManga needs specific fulltext ft_* options in config.");

        return;
    }
    if (qr.next()) {
        bool okconv;
        int ftlen = qr.value(0).toInt(&okconv);
        if (!okconv || (ftlen>2)) {
            if (!silent)
                emit errorMsg(tr(FT_DETAILS));
            problems[tr("MySQL config fulltext")] = tr(FT_DETAILS);
        }
    }
}

void ZDB::sqlCreateTables()
{
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    checkConfigOpts(db,false);
    QSqlQuery qr(db);

    if (isMySQL(db)) {
        if (!qr.exec("CREATE TABLE IF NOT EXISTS `albums` ("
                     "`id` int(11) NOT NULL AUTO_INCREMENT,"
                     "`name` varchar(2048) NOT NULL,"
                     "PRIMARY KEY (`id`)"
                     ") ENGINE=MyISAM DEFAULT CHARSET=utf8")) {
            emit errorMsg(tr("Unable to create table `albums`\n\n%1\n%2")
                          .arg(qr.lastError().databaseText(),qr.lastError().driverText()));
            problems[tr("Create tables - albums")] = tr("Unable to create table `albums`.\n"
                                                        "CREATE TABLE query failed.");
            sqlCloseBase(db);
            return;
        }

        if (!qr.exec("CREATE TABLE IF NOT EXISTS `ignored_files` ("
                     "`id` int(11) NOT NULL AUTO_INCREMENT,"
                     "`filename` varchar(16383) NOT NULL,"
                     "PRIMARY KEY (`id`)"
                     ") ENGINE=MyISAM DEFAULT CHARSET=utf8")) {
            emit errorMsg(tr("Unable to create table `ignored_files`\n\n%1\n%2")
                          .arg(qr.lastError().databaseText(),qr.lastError().driverText()));
            problems[tr("Create tables - ignored_files")] = tr("Unable to create table `ignored_files`.\n"
                                                               "CREATE TABLE query failed.");
            sqlCloseBase(db);
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
                     "`preferredRendering` int(11) default 0,"
                     "PRIMARY KEY (`id`),"
                     "FULLTEXT KEY `name_ft` (`name`)"
                     ") ENGINE=MyISAM  DEFAULT CHARSET=utf8")) {
            emit errorMsg(tr("Unable to create table `files`\n\n%1\n%2")
                          .arg(qr.lastError().databaseText(),qr.lastError().driverText()));
            problems[tr("Create tables - files")] = tr("Unable to create table `files`.\n"
                                                       "CREATE TABLE query failed.");
            sqlCloseBase(db);
            return;
        }
    } else {
        // TODO: add support for sqlite
    }
    sqlCloseBase(db);
    return;
}

QSqlDatabase ZDB::sqlOpenBase()
{
    if (dbUser.isEmpty()) return QSqlDatabase();
    QSqlDatabase db;

    bool useMySQL = !dbHost.isEmpty() && !dbBase.isEmpty();
    if (useMySQL)
        db = QSqlDatabase::addDatabase("QMYSQL",QUuid::createUuid().toString());
    else
        db = QSqlDatabase::addDatabase("QSQLITE",QUuid::createUuid().toString());

    if (db.isValid()) {
        if (useMySQL) {
            db.setHostName(dbHost);
            db.setDatabaseName(dbBase);
            db.setUserName(dbUser);
            db.setPassword(dbPass);
            if (!db.open()) {
                db = QSqlDatabase();
                emit errorMsg(tr("Unable to open MySQL connection. Check login info.\n%1\n%2")
                              .arg(db.lastError().driverText(),db.lastError().databaseText()));
                problems[tr("Connection")] = tr("Unable to connect to MySQL.\n"
                                                "Check credentials and MySQL running.");
            }
        } else {
            // TODO: open sqlite base
        }
    }

    return db;
}

void ZDB::sqlCloseBase(QSqlDatabase &db)
{
    if (db.isOpen())
        db.close();
}

void ZDB::sqlUpdateIgnoredFiles(QSqlDatabase &db)
{
    QStringList sl;
    QSqlQuery qr("SELECT filename FROM ignored_files",db);
    while (qr.next())
        sl << qr.value(0).toString();

    ignoredFiles = sl;
}

void ZDB::sqlGetAlbums()
{
    QStringList result;
    result.clear();
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    QSqlQuery qr("SELECT name FROM `albums` ORDER BY name ASC",db);
    while (qr.next())
        result << qr.value(0).toString();

    sqlUpdateIgnoredFiles(db);

    sqlCloseBase(db);

    foreach (const QString& title, dynAlbums.keys())
        result << QString("# %1").arg(title);
    result << QString("% Deleted");

    emit gotAlbums(result);
}

void ZDB::sqlGetFiles(const QString &album, const QString &search, const Z::Ordering sortOrder,
                      const bool reverseOrder)
{
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    QTime tmr;
    tmr.start();

    QString tqr = QString("SELECT files.name, filename, cover, pagesCount, fileSize, fileMagic, fileDT, "
                         "addingDT, files.id, albums.name, files.preferredRendering "
                         "FROM files LEFT JOIN albums ON files.album=albums.id ");
    bool specQuery = false;
    bool checkFS = false;

    bool albumBind = false;
    bool searchBind = false;

    if (!album.isEmpty()) {
        if (album.startsWith("# ") && dynAlbums.keys().contains(album.mid(2))) {
            specQuery = true;
            tqr += dynAlbums.value(album.mid(2));
        } else if (album.startsWith("% Deleted")) {
            specQuery = true;
            checkFS = true;
        } else {
            tqr += QString("WHERE (album="
                           "  (SELECT id FROM albums WHERE (name = ?))"
                           ") ");
            albumBind = true;
        }
    } else if (!search.isEmpty()) {
        QString sqr;
        if (isMySQL(db))
            sqr = prepareSearchQuery(search);
        if (sqr.isEmpty()) {
            sqr = QString("WHERE (files.name LIKE ?) ");
            searchBind = true;
        }
        tqr += sqr;
    }

    if (!specQuery) {
        tqr+="ORDER BY ";
        tqr+=Z::sqlColumns.value(sortOrder);
        if (reverseOrder) tqr+=" DESC";
        else tqr+=" ASC";
    }

    int idx = 0;

    QSqlQuery qr(db);
    qr.prepare(tqr);
    if (albumBind)
        qr.addBindValue(album);
    if (searchBind)
        qr.addBindValue(search);

    if (qr.exec()) {
        while (qr.next()) {
            QImage p = QImage();
            QByteArray ba = qr.value(2).toByteArray();
            if (!ba.isEmpty())
                p.loadFromData(ba);

            QString fileName = qr.value(1).toString();

            if (checkFS) {
                QFileInfo fi(fileName);
                if (fi.exists()) continue;
            }

            int prefRendering = qr.value(10).toInt();
            preferredRendering[fileName] = prefRendering;

            emit gotFile(SQLMangaEntry(qr.value(0).toString(),
                                       fileName,
                                       qr.value(9).toString(),
                                       p,
                                       qr.value(3).toInt(),
                                       qr.value(4).toInt(),
                                       qr.value(5).toString(),
                                       qr.value(6).toDateTime(),
                                       qr.value(7).toDateTime(),
                                       qr.value(8).toInt(),
                                       static_cast<Z::PDFRendering>(prefRendering)),
                    sortOrder, reverseOrder);
            QApplication::processEvents();
            idx++;
        }
    } else
        qDebug() << qr.lastError().databaseText() << qr.lastError().driverText();
    sqlCloseBase(db);
    emit filesLoaded(idx,tmr.elapsed());
    return;
}

void ZDB::sqlChangeFilePreview(const QString &fileName, const int pageNum)
{
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    QString fname(fileName);
    bool dynManga = fname.startsWith("#DYN#");
    if (dynManga)
        fname.remove(QRegExp("^#DYN#"));

    QSqlQuery qr(db);
    qr.prepare("SELECT name FROM files WHERE (filename=?)");
    qr.addBindValue(fname);
    if (!qr.exec()) {
        qDebug() << "file search query failed";
        sqlCloseBase(db);
        return;
    }
    if (!qr.next()) {
        emit errorMsg("Opened file not found in library.");
        sqlCloseBase(db);
        return;
    }

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
    qr.prepare("UPDATE files SET cover=? WHERE (filename=?)");
    qr.bindValue(0,pba);
    qr.bindValue(1,fname);
    if (!qr.exec()) {
        QString msg = tr("Unable to change cover for '%1'.\n%2\n%3")
                .arg(fname,qr.lastError().databaseText(),qr.lastError().driverText());
        qDebug() << msg;
        emit errorMsg(msg);
    }
    pba.clear();
    za->closeFile();
    za->setParent(NULL);
    delete za;

    sqlCloseBase(db);
    return;
}

void ZDB::sqlRescanIndexedDirs()
{
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    QStringList dirs;
    dirs.clear();

    QSqlQuery qr("SELECT filename "
                 "FROM files "
                 "WHERE NOT(fileMagic='DYN')",db);
    while (qr.next()) {
        QFileInfo fi(qr.value(0).toString());
        if (!fi.exists()) continue;
        if (!dirs.contains(fi.absoluteDir().absolutePath()))
            dirs << fi.absoluteDir().absolutePath();
    }

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

    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    QSqlQuery qr(db);
    qr.prepare("UPDATE files SET pagesCount=?, fileSize=?, fileMagic=?, fileDT=? "
               "WHERE (filename=?)");

    qr.bindValue(0,za->getPageCount());
    if (dynManga)
        qr.bindValue(1,0);
    else
        qr.bindValue(1,fi.size());
    qr.bindValue(2,za->getMagic());
    qr.bindValue(3,fi.created());
    qr.bindValue(4,fname);

    za->closeFile();
    za->setParent(NULL);
    delete za;

    if (!qr.exec())
        qDebug() << fname << "unable to update file stats" <<
                    qr.lastError().databaseText() << qr.lastError().driverText();

    sqlCloseBase(db);
    return;
}

bool isFileInTable(QSqlDatabase &db, const QString& filename, const QString& table, bool *error)
{
    *error=false;

    QSqlQuery qr(db);
    qr.prepare(QString("SELECT id FROM %1 WHERE filename=?").arg(table));
    qr.addBindValue(filename);
    if (qr.exec()) {
        if (qr.size()>0) return true;
    } else {
        *error=true;
    }
    return false;
}

void ZDB::sqlSearchMissingManga()
{
    QStringList foundFiles;
    foundFiles.clear();

    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

#ifdef _WIN32
    const bool isWin32 = true;
#else
    const bool isWin32 = false;
#endif
    bool useSimple = isWin32 || !isMySQL(db);

    QStringList filenames;

    foreach (const QString& d, indexedDirs) {
        QDir dir(d);
        QFileInfoList fl = dir.entryInfoList(QStringList("*"), QDir::Files | QDir::Readable);
        for (int i=0;i<fl.count();i++)
            if (useSimple) {
                if (fl.at(i).isReadable() && fl.at(i).isFile()) {
                    bool err1,err2;
                    if (!isFileInTable(db,fl.at(i).absoluteFilePath(),"files",&err1) &&
                            !isFileInTable(db,fl.at(i).absoluteFilePath(),"ignored_files",&err2)) {
                        if (!err1 && !err2) {
                            foundFiles << fl.at(i).absoluteFilePath();
                        } else {
                            qDebug() << db.lastError().databaseText() << db.lastError().driverText();
                            sqlCloseBase(db);
                            return;
                        }
                    }
                }
            } else {
                filenames << fl.at(i).absoluteFilePath();
            }
    }

    if (!useSimple) {
        QSqlQuery qr(db);
        qr.prepare("CREATE TEMPORARY TABLE IF NOT EXISTS `tmp_exfiles` ("
                   "`filename` varchar(16383), INDEX ifilename (filename)"
                   ") ENGINE=MyISAM DEFAULT CHARSET=utf8");
        if (!qr.exec()) {
            emit errorMsg(tr("Unable to create temporary table `tmp_exfiles`\n\n%1\n%2")
                          .arg(qr.lastError().databaseText(),qr.lastError().driverText()));
            problems[tr("Create tables - tmp_exfiles")] = tr("Unable to create temporary table `tmp_exfiles`.\n"
                                                             "CREATE TABLE query failed.");
            sqlCloseBase(db);
            return;
        }

        qr.prepare("TRUNCATE TABLE `tmp_exfiles`");
        if (!qr.exec()) {
            emit errorMsg(tr("Unable to truncate temporary table `tmp_exfiles`\n\n%1\n%2")
                          .arg(qr.lastError().databaseText(),qr.lastError().driverText()));
            problems[tr("Truncate table - tmp_exfiles")] = tr("Unable to truncate temporary table `tmp_exfiles`.\n"
                                                              "TRUNCATE TABLE query failed.");
            sqlCloseBase(db);
            return;
        }

        while (!filenames.isEmpty()) {
            QVariantList sl;
            while (sl.count()<32 && !filenames.isEmpty())
                sl << filenames.takeFirst();
            if (!sl.isEmpty()) {
                qr.prepare("INSERT INTO `tmp_exfiles` VALUES (?)");
                qr.addBindValue(sl);
                if (!qr.execBatch()) {
                    emit errorMsg(tr("Unable to load data to table `tmp_exfiles`\n\n%1\n%2")
                                  .arg(qr.lastError().databaseText(),qr.lastError().driverText()));
                    problems[tr("Load data infile - tmp_exfiles")] =
                            tr("Unable to load data to temporary table `tmp_exfiles`.\n"
                               "LOAD DATA INFILE query failed.");
                    sqlCloseBase(db);
                    return;
                }
            }
        }

        qr.prepare("SELECT tmp_exfiles.filename FROM tmp_exfiles "
                   "LEFT JOIN files ON tmp_exfiles.filename=files.filename "
                   "LEFT JOIN ignored_files ON tmp_exfiles.filename=ignored_files.filename "
                   "WHERE (files.filename IS NULL) AND (ignored_files.filename IS NULL)");
        if (qr.exec()) {
            while (qr.next())
                foundFiles << qr.value(0).toString();
        } else
            qDebug() << qr.lastError().databaseText() << qr.lastError().driverText();

        qr.prepare("DROP TABLE `tmp_exfiles`");
        if (!qr.exec())
            qDebug() << qr.lastError().databaseText() << qr.lastError().driverText();
    }
    sqlCloseBase(db);

    emit foundNewFiles(foundFiles);
}

void ZDB::sqlAddIgnoredFiles(const QStringList& files)
{
    sqlInsertIgnoredFilesPrivate(files,false);
}

void ZDB::sqlSetIgnoredFiles(const QStringList& files)
{
    sqlInsertIgnoredFilesPrivate(files,true);
}

void ZDB::sqlInsertIgnoredFilesPrivate(const QStringList &files, bool cleanTable)
{
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    QSqlQuery qr(db);

    if (cleanTable) {
        qr.prepare("TRUNCATE TABLE `ignored_files`");
        if (!qr.exec()) {
            emit errorMsg(tr("Unable to truncate table `ignored_files`\n\n%1\n%2")
                          .arg(qr.lastError().databaseText(),qr.lastError().driverText()));
            problems[tr("Truncate table - ignored_files")] = tr("Unable to truncate table `ignored_files`.\n"
                                                                "TRUNCATE TABLE query failed.");
            sqlCloseBase(db);
            return;
        }
        ignoredFiles.clear();
    }

    foreach (const QString& file, files) {
        qr.prepare("INSERT INTO ignored_files (filename) VALUES (?)");
        qr.addBindValue(file);
        if (!qr.exec()) {
            emit errorMsg(tr("Unable to add ignored file `%1`\n%2\n%3").
                          arg(file,qr.lastError().databaseText(),qr.lastError().driverText()));
            sqlCloseBase(db);
            return;
        }
        ignoredFiles << file;
    }
    sqlCloseBase(db);
}

bool ZDB::isMySQL(QSqlDatabase &db)
{
    if (!db.isValid() || db.driver()==NULL) return false;
    return db.driver()->dbmsType()==QSqlDriver::MySqlServer;
}

QStringList ZDB::sqlGetIgnoredFiles() const
{
    return ignoredFiles;
}

Z::PDFRendering ZDB::getPreferredRendering(const QString &filename) const
{
    if (preferredRendering.contains(filename))
        return static_cast<Z::PDFRendering>(preferredRendering.value(filename));
    else
        return Z::PDFRendering::Autodetect;
}

bool ZDB::sqlSetPreferredRendering(const QString &filename, int mode)
{
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return Z::PDFRendering::Autodetect;

    QSqlQuery qr(db);
    qr.prepare("UPDATE files SET preferredRendering=? WHERE filename=?");
    qr.bindValue(0,mode);
    qr.bindValue(1,filename);
    bool res = true;
    if (!qr.exec()) {
        QString msg = tr("Unable to change preferred rendering for '%1'.\n%2\n%3").
                      arg(filename,qr.lastError().databaseText(),qr.lastError().driverText());
        qDebug() << msg;
        emit errorMsg(msg);
        res = false;
    }
    preferredRendering[filename] = mode;
    sqlCloseBase(db);

    return res;

}

void ZDB::sqlGetTablesDescription()
{
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;
    if (db.driver()==NULL) {
        sqlCloseBase(db);
        return;
    }

    QSqlRecord rec = db.driver()->record("files");
    if (rec.isEmpty()) return;

    QStringList names, types;
    int nl = 0; int tl = 0;
    for (int i=0; i < rec.count(); i++)
    {
        names << rec.field(i).name();
        switch (rec.field(i).type()) {
            case QVariant::Int:
            case QVariant::UInt:
            case QVariant::ULongLong:
            case QVariant::LongLong:
                types << "NUMERIC";
                break;
            case QVariant::String:
            case QVariant::StringList:
                types << "STRING";
                break;
            case QVariant::Icon:
            case QVariant::Image:
            case QVariant::Bitmap:
            case QVariant::Pixmap:
            case QVariant::ByteArray:
                types << "BLOB";
                break;
            case QVariant::Date:
                types << "DATE";
                break;
            case QVariant::DateTime:
                types << "DATETIME";
                break;
            case QVariant::Time:
                types << "TIME";
                break;
            case QVariant::Double:
                types << "DOUBLE";
                break;
            default:
                types << "unknown";
                break;
        }
        if (names.last().length()>nl)
            nl = names.last().length();

        if (types.last().length()>tl)
            tl = types.last().length();
    }

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
    qDebug() << res.join('\n');
    emit gotTablesDescription(res.join('\n'));
}

void ZDB::sqlAddFiles(const QStringList& aFiles, const QString& album)
{
    if (album.isEmpty()) {
        emit errorMsg(tr("Album name cannot be empty"));
        return;
    }
    if (album.startsWith("#") || album.startsWith("%")) {
        emit errorMsg(tr("Reserved character in album name"));
        return;
    }
    if (aFiles.first().startsWith("###DYNAMIC###")) {
        fsAddImagesDir(aFiles.last(),album);
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

    QSqlQuery qr(db);

    qr.prepare("SELECT id FROM albums WHERE (name=?)");
    qr.addBindValue(album);
    if (qr.exec()) {
        if (qr.next())
            ialbum = qr.value(0).toInt();
    }

    if (ialbum==-1) {
        qr.prepare("INSERT INTO albums (name) VALUES (?)");
        qr.addBindValue(album);
        if (!qr.exec()) {
            emit errorMsg(tr("Unable to create album `%1`\n%2\n%3").
                          arg(album,qr.lastError().databaseText(),qr.lastError().driverText()));
            sqlCloseBase(db);
            return;
        }
        bool ok;
        ialbum = qr.lastInsertId().toInt(&ok);
        if (!ok) {
            qr.prepare("SELECT id FROM albums WHERE (name=?)");
            qr.addBindValue(album);
            if (qr.exec()) {
                if (qr.next())
                    ialbum = qr.value(0).toInt();
            }
        }
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
        qr.prepare("INSERT INTO files (name, filename, album, cover, pagesCount, fileSize, "
                   "fileMagic, fileDT, addingDT) VALUES (?, ?, ?, ?, ?, ?, ?, ?, NOW())");
        qr.bindValue(0,fi.completeBaseName());
        qr.bindValue(1,files.at(i));
        qr.bindValue(2,ialbum);
        qr.bindValue(3,pba);
        qr.bindValue(4,za->getPageCount());
        qr.bindValue(5,fi.size());
        qr.bindValue(6,za->getMagic());
        qr.bindValue(7,fi.created());
        if (!qr.exec())
            qDebug() << files.at(i) << "unable to add" <<
                        qr.lastError().databaseText() << qr.lastError().driverText();
        else
            cnt++;
        pba.clear();
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
    QFileInfoList fl = d.entryInfoList(QStringList("*"), QDir::Files | QDir::Readable);
    filterSupportedImgFiles(fl);
    QStringList files;
    for (int i=0;i<fl.count();i++)
        files << fl.at(i).absoluteFilePath();

    if (files.isEmpty()) {
        emit errorMsg("Could not find supported image files in specified directory");
        return;
    }

    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    // get album number
    int ialbum = -1;

    QSqlQuery qr(db);
    qr.prepare("SELECT id FROM albums WHERE (name=?)");
    qr.addBindValue(album);
    if (qr.exec()) {
        if (qr.next())
            ialbum = qr.value(0).toInt();
    }

    if (ialbum==-1) {
        qr.prepare("INSERT INTO albums (name) VALUES (?)");
        qr.addBindValue(album);
        if (!qr.exec()) {
            emit errorMsg(tr("Unable to create album `%1`\n%2\n%3").
                          arg(album,qr.lastError().databaseText(),qr.lastError().driverText()));
            sqlCloseBase(db);
            return;
        }
        bool ok;
        ialbum = qr.lastInsertId().toInt(&ok);
        if (!ok) {
            qr.prepare("SELECT id FROM albums WHERE (name=?)");
            qr.addBindValue(album);
            if (qr.exec()) {
                if (qr.next())
                    ialbum = qr.value(0).toInt();
            }
        }
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
    pba.clear();
    if (!p.isNull()) {
        p = p.scaled(maxPreviewSize,maxPreviewSize,Qt::KeepAspectRatio,Qt::SmoothTransformation);
        QBuffer buf(&pba);
        buf.open(QIODevice::WriteOnly);
        p.save(&buf, "JPG");
        buf.close();
        p = QImage();
    }

    // add dynamic album to base
    int cnt = 0;
    qr.prepare("INSERT INTO files (name, filename, album, cover, pagesCount, fileSize, "
               "fileMagic, fileDT, addingDT) VALUES (?, ?, ?, ?, ?, ?, ?, ?, NOW())");
    qr.bindValue(0,d.dirName());
    qr.bindValue(1,d.absolutePath());
    qr.bindValue(2,ialbum);
    qr.bindValue(3,pba);
    qr.bindValue(4,files.count());
    qr.bindValue(5,0);
    qr.bindValue(6,QString("DYN"));
    qr.bindValue(7,fi.created());
    if (!qr.exec())
        qDebug() << "unable to add dynamic album" << dir <<
                    qr.lastError().databaseText() << qr.lastError().driverText();
    else
        cnt++;
    pba.clear();

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
    }
    return QString();
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
    delqr+=")";

    QString fsqr = QString("SELECT filename,fileMagic FROM files WHERE id IN (");
    for (int i=0;i<dbids.count();i++) {
        if (i>0) fsqr+=",";
        fsqr+=QString("%1").arg(dbids.at(i));
    }
    fsqr+=")";

    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    bool okdel = true;
    QSqlQuery qr(db);

    if (fullDelete) {
        qr.prepare(fsqr);
        if (qr.exec()) {
            while (qr.next()) {
                QString fname = qr.value(0).toString();
                QString magic = qr.value(1).toString();
                if (magic!="DYN")
                    if (!QFile::remove(fname))
                        okdel = false;
            }
        }
    }

    qr.prepare(delqr);
    if (!qr.exec()) {
        emit errorMsg(tr("Unable to delete manga\n%1\n%2").
                      arg(qr.lastError().databaseText(), qr.lastError().driverText()));
        sqlCloseBase(db);
        return;
    }
    sqlCloseBase(db);
    sqlDelEmptyAlbums();

    emit deleteItemsFromModel(dbids);

    if (!okdel && fullDelete)
        emit errorMsg(tr("Errors occured, some files not deleted from filesystem or not found.\n%1\n%2").
                      arg(db.lastError().databaseText(), db.lastError().driverText()));
}

void ZDB::sqlDelEmptyAlbums()
{
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;
    QSqlQuery qr(db);
    if (!qr.exec("DELETE FROM albums WHERE id NOT IN (SELECT DISTINCT album FROM files)"))
        qDebug() << qr.lastError().databaseText() << qr.lastError().driverText();
    sqlCloseBase(db);
    emit albumsListUpdated();
}

void ZDB::sqlRenameAlbum(const QString &oldName, const QString &newName)
{
    if (oldName==newName || oldName.isEmpty() || newName.isEmpty()) return;
    if (newName.startsWith("#") || oldName.startsWith("#")) return;
    if (newName.startsWith("%") || oldName.startsWith("%")) return;

    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    QSqlQuery qr(db);
    qr.prepare("UPDATE albums SET name=? WHERE name=?");
    qr.bindValue(0,newName);
    qr.bindValue(1,oldName);
    if (!qr.exec()) {
        qDebug() << "Unable to rename album"
                 << qr.lastError().databaseText() << qr.lastError().driverText();
        emit errorMsg(tr("Unable to rename album `%1`\n%2\n%3").
                      arg(oldName,qr.lastError().databaseText(), qr.lastError().driverText()));
    }
    sqlCloseBase(db);
    emit albumsListUpdated();
}

void ZDB::sqlDelAlbum(const QString &album)
{
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    QSqlQuery qr(db);
    qr.prepare("SELECT id FROM `albums` WHERE name=?");
    qr.addBindValue(album);
    int id = -1;

    if (qr.exec()) {
        if (qr.next())
            id = qr.value(0).toInt();
    }
    if (id==-1) {
        emit errorMsg(tr("Album '%1' not found").arg(album));
        sqlCloseBase(db);
        return;
    }

    qr.prepare("DELETE FROM files WHERE album=?");
    qr.addBindValue(id);
    if (!qr.exec()) {
        QString msg = tr("Unable to delete manga\n%1\n%2").
                      arg(qr.lastError().databaseText(), qr.lastError().driverText());
        emit errorMsg(msg);
        qDebug() << msg;
        sqlCloseBase(db);
        return;
    }
    sqlCloseBase(db);
    sqlDelEmptyAlbums();
}
