#include <QApplication>
#include <QTemporaryFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QBuffer>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDriver>
#include <QSqlRecord>
#include <QSqlField>
#include <QVariant>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <QDebug>
#include "zdb.h"

ZDB::ZDB(QObject *parent) :
    QObject(parent)
{
    m_dbHost = QSL("localhost");
    m_dbBase = QSL("qmanga");
    m_dbUser = QString();
    m_dbPass = QString();
    m_wasCanceled = false;
    m_indexedDirs.clear();
    m_problems.clear();
    m_ignoredFiles.clear();
    m_preferredRendering.clear();
}

int ZDB::getAlbumsCount()
{
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return 0;

    QSqlQuery qr(QSL("SELECT COUNT(name) FROM `albums` ORDER BY name ASC"),db);
    int cnt = 0;
    while (qr.next())
        cnt = qr.value(0).toInt();

    sqlCloseBase(db);
    return cnt;
}

ZStrMap ZDB::getDynAlbums() const
{
    return m_dynAlbums;
}

Z::Ordering ZDB::getDynAlbumOrdering(const QString& album, Qt::SortOrder &order) const
{
    if (!album.startsWith(QSL("# "))) return Z::ordUndefined;
    const QString name = album.mid(2);

    order = Qt::AscendingOrder;
    const QString qr = m_dynAlbums.value(name,QString());
    if (!qr.isEmpty()) {
        QRegularExpression rx(QSL(R"(ORDER\sBY\s(\S+)\s(ASC|DESC)?)"));
        QRegularExpressionMatch mrx = rx.match(qr);
        if (mrx.hasMatch()) {
            QString col = mrx.captured(1).trimmed();
            QString ord = mrx.captured(2).trimmed();

            if (!ord.isEmpty() &&
                    (ord.compare(QSL("DESC"),Qt::CaseInsensitive)==0))
                order = Qt::DescendingOrder;

            if (!col.isEmpty()) {
                if (!col.at(0).isLetterOrNumber())
                    col.remove(col.at(0));
                const auto columns = ZGenericFuncs::getSqlColumns();
                for(auto it = columns.constKeyValueBegin(),
                    end = columns.constKeyValueEnd(); it != end; ++it) {
                    if (col.compare((*it).second,Qt::CaseInsensitive)==0) {
                        return (*it).first;
                    }
                }
            }
        }
    }
    return Z::ordUndefined;
}

ZStrHash ZDB::getConfigProblems() const
{
    return m_problems;
}

void ZDB::setCredentials(const QString &host, const QString &base, const QString &user, const QString &password)
{
    m_dbHost = host;
    m_dbBase = base;
    m_dbUser = user;
    m_dbPass = password;
}

void ZDB::setDynAlbums(const ZStrMap &albums)
{
    m_dynAlbums.clear();
    m_dynAlbums = albums;
}

void ZDB::sqlCheckBase()
{
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    sqlCheckBasePriv(db,false);

    sqlCloseBase(db);

    Q_EMIT baseCheckComplete();
}

bool ZDB::sqlCheckBasePriv(QSqlDatabase& db, bool silent)
{
    static const QStringList tables { QSL("files"), QSL("albums"), QSL("ignored_files") };

    int cnt=0;
    const QStringList list = db.tables(QSql::Tables);
    for (const QString& s : list) {
        if (tables.contains(s,Qt::CaseInsensitive))
            cnt++;
    }

    bool noTables = (cnt!=3);

    if (noTables) {
        if (!silent)
            Q_EMIT needTableCreation();
        return false;
    }

    if (silent) return true;

    return checkTablesParams(db);
}

bool ZDB::checkTablesParams(QSqlDatabase &db)
{
    if (sqlDbEngine(db)!=Z::dbmsMySQL) return true;

    QSqlQuery qr(db);
    QStringList cols;

    // Add parent column to albums
    qr.exec(QSL("SHOW COLUMNS FROM albums"));
    while (qr.next())
        cols << qr.value(0).toString();

    if (!cols.contains(QSL("parent"),Qt::CaseInsensitive)) {
        if (!qr.exec(QSL("ALTER TABLE `albums` ADD COLUMN `parent` "
                                    "INT(11) DEFAULT -1"))) {
            qWarning() << tr("Unable to add column for albums table.")
                     << qr.lastError().databaseText() << qr.lastError().driverText();

            m_problems[tr("Adding column")] = tr("Unable to add column for qmanga albums table.\n"
                                               "ALTER TABLE query failed.");

            return false;
        }
    }

    // Check for preferred rendering column
    qr.exec(QSL("SHOW COLUMNS FROM files"));
    while (qr.next())
        cols << qr.value(0).toString();

    if (!cols.contains(QSL("preferredRendering"),Qt::CaseInsensitive)) {
        if (!qr.exec(QSL("ALTER TABLE `files` ADD COLUMN `preferredRendering` "
                                    "INT(11) DEFAULT 0"))) {
            qWarning() << tr("Unable to add column for files table.")
                     << qr.lastError().databaseText() << qr.lastError().driverText();

            m_problems[tr("Adding column")] = tr("Unable to add column for qmanga files table.\n"
                                               "ALTER TABLE query failed.");

            return false;
        }
    }

    // Add fulltext index for manga names
    if (!qr.exec(QSL("SELECT index_type FROM information_schema.statistics "
                 "WHERE table_schema=DATABASE() AND table_name='files' AND index_name='name_ft'"))) {
        qWarning() << tr("Unable to get indexes list.")
                 << qr.lastError().databaseText() << qr.lastError().driverText();

        m_problems[tr("Check for indexes")] = tr("Unable to check indexes for qmanga tables.\n"
                                               "SELECT query failed.\n");

        return false;
    }
    QString idxn;
    if (qr.next())
        idxn = qr.value(0).toString();
    if (idxn.isEmpty()) {
        if (!qr.exec(QSL("ALTER TABLE files ADD FULLTEXT INDEX name_ft(name)"))) {
            qWarning() << tr("Unable to add fulltext index for files table.")
                     << qr.lastError().databaseText() << qr.lastError().driverText();

            m_problems[tr("Creating indexes")] = tr("Unable to add FULLTEXT index for qmanga files table.\n"
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
    static const QString ftDetails = QSL("1. Edit my.cnf file and save\n"
                                         "--> ft_stopword_file = \"\" (or link an empty file \"empty_stopwords.txt\")\n"
                                         "--> ft_min_word_len = 2\n"
                                         "2. Restart your server (this cannot be done dynamically)\n"
                                         "3. Change the table engine (if needed) - ALTER TABLE tbl_name ENGINE = MyISAM;\n"
                                         "4. Perform repair - REPAIR TABLE tbl_name QUICK;");

    if (sqlDbEngine(db)!=Z::dbmsMySQL) return;

    QSqlQuery qr(db);
    if (!qr.exec(QSL("SELECT @@ft_min_word_len"))) {
        qWarning() << tr("Unable to check MySQL config options");

        m_problems[tr("MySQL config")] = tr("Unable to check my.cnf options via global variables.\n"
                                          "SELECT query failed.\n"
                                          "QManga needs specific fulltext ft_* options in config.");

        return;
    }
    if (qr.next()) {
        bool okconv = false;
        int ftlen = qr.value(0).toInt(&okconv);
        if (!okconv || (ftlen>2)) {
            if (!silent)
                Q_EMIT errorMsg(ftDetails);
            m_problems[tr("MySQL config fulltext")] = ftDetails;
        }
    }
}

void ZDB::sqlCreateTables()
{
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    checkConfigOpts(db,false);
    QSqlQuery qr(db);

    if (sqlDbEngine(db)==Z::dbmsMySQL) {
        if (!qr.exec(QSL("CREATE TABLE IF NOT EXISTS `albums` ("
                     "`id` int(11) NOT NULL AUTO_INCREMENT,"
                     "`name` varchar(2048) NOT NULL,"
                     "`parent` int(11) default -1,"
                     "PRIMARY KEY (`id`)"
                     ") ENGINE=MyISAM DEFAULT CHARSET=utf8"))) {
            Q_EMIT errorMsg(tr("Unable to create table `albums`\n\n%1\n%2")
                          .arg(qr.lastError().databaseText(),qr.lastError().driverText()));
            m_problems[tr("Create tables - albums")] = tr("Unable to create table `albums`.\n"
                                                        "CREATE TABLE query failed.");
            sqlCloseBase(db);
            return;
        }

        if (!qr.exec(QSL("CREATE TABLE IF NOT EXISTS `ignored_files` ("
                     "`id` int(11) NOT NULL AUTO_INCREMENT,"
                     "`filename` varchar(16383) NOT NULL,"
                     "PRIMARY KEY (`id`)"
                     ") ENGINE=MyISAM DEFAULT CHARSET=utf8"))) {
            Q_EMIT errorMsg(tr("Unable to create table `ignored_files`\n\n%1\n%2")
                          .arg(qr.lastError().databaseText(),qr.lastError().driverText()));
            m_problems[tr("Create tables - ignored_files")] = tr("Unable to create table `ignored_files`.\n"
                                                               "CREATE TABLE query failed.");
            sqlCloseBase(db);
            return;
        }
        if (!qr.exec(QSL("CREATE TABLE IF NOT EXISTS `files` ("
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
                     ") ENGINE=MyISAM  DEFAULT CHARSET=utf8"))) {
            Q_EMIT errorMsg(tr("Unable to create table `files`\n\n%1\n%2")
                          .arg(qr.lastError().databaseText(),qr.lastError().driverText()));
            m_problems[tr("Create tables - files")] = tr("Unable to create table `files`.\n"
                                                       "CREATE TABLE query failed.");
            sqlCloseBase(db);
            return;
        }
    } else if (sqlDbEngine(db)==Z::dbmsSQLite){
        if (!qr.exec(QSL("CREATE TABLE IF NOT EXISTS `albums` ("
                     "`id` INTEGER PRIMARY KEY AUTOINCREMENT,"
                     "`name` TEXT,"
                     "`parent` INTEGER DEFAULT -1)"))) {
            Q_EMIT errorMsg(tr("Unable to create table `albums`\n\n%1\n%2")
                          .arg(qr.lastError().databaseText(),qr.lastError().driverText()));
            m_problems[tr("Create tables - albums")] = tr("Unable to create table `albums`.\n"
                                                        "CREATE TABLE query failed.");
            sqlCloseBase(db);
            return;
        }

        if (!qr.exec(QSL("CREATE TABLE IF NOT EXISTS `ignored_files` ("
                     "`id` INTEGER PRIMARY KEY,"
                     "`filename` TEXT)"))) {
            Q_EMIT errorMsg(tr("Unable to create table `ignored_files`\n\n%1\n%2")
                          .arg(qr.lastError().databaseText(),qr.lastError().driverText()));
            m_problems[tr("Create tables - ignored_files")] = tr("Unable to create table `ignored_files`.\n"
                                                               "CREATE TABLE query failed.");
            sqlCloseBase(db);
            return;
        }
        if (!qr.exec(QSL("CREATE TABLE IF NOT EXISTS `files` ("
                     "`id` INTEGER PRIMARY KEY,"
                     "`name` TEXT,"
                     "`filename` TEXT,"
                     "`album` INTEGER,"
                     "`cover` BLOB,"
                     "`pagesCount` INTEGER,"
                     "`fileSize` INTEGER,"
                     "`fileMagic` TEXT,"
                     "`fileDT` TEXT,"
                     "`addingDT` TEXT,"
                     "`preferredRendering` INTEGER DEFAULT 0)"))) {
            Q_EMIT errorMsg(tr("Unable to create table `files`\n\n%1\n%2")
                          .arg(qr.lastError().databaseText(),qr.lastError().driverText()));
            m_problems[tr("Create tables - files")] = tr("Unable to create table `files`.\n"
                                                       "CREATE TABLE query failed.");
            sqlCloseBase(db);
            return;
        }
    } else {
        Q_EMIT errorMsg(tr("Unable to create tables. Unknown DB engine"));
        m_problems[tr("Create tables")] = tr("Unable to create tables.\n"
                                           "Unknown DB engine.");
    }
    sqlCloseBase(db);
}

QSqlDatabase ZDB::sqlOpenBase()
{
    QSqlDatabase db;

    if (zF->global()==nullptr) return db;

    if (zF->global()->getDbEngine()==Z::dbmsMySQL) {
        db = QSqlDatabase::addDatabase(QSL("QMYSQL"),QUuid::createUuid().toString());
        if (!db.isValid()) {
            db = QSqlDatabase();
            Q_EMIT errorMsg(tr("Unable to create MySQL driver instance."));
            m_problems[tr("Connection")] = tr("Unable to create MySQL driver instance.");
            return db;
        }

        db.setHostName(m_dbHost);
        db.setDatabaseName(m_dbBase);
        db.setUserName(m_dbUser);
        db.setPassword(m_dbPass);

    } else if (zF->global()->getDbEngine()==Z::dbmsSQLite) {
        db = QSqlDatabase::addDatabase(QSL("QSQLITE"),QUuid::createUuid().toString());
        if (!db.isValid()) {
            db = QSqlDatabase();
            Q_EMIT errorMsg(tr("Unable to create SQLite driver instance."));
            m_problems[tr("Connection")] = tr("Unable to create SQLite driver instance.");
            return db;
        }

        QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dbDir(dir);
        if (!dbDir.exists()) {
            if (!dbDir.mkpath(dir)) {
                db = QSqlDatabase();
                Q_EMIT errorMsg(tr("Unable to create SQLite database file. Check file permissions.\n%1")
                              .arg(dir));
                m_problems[tr("Connection")] = tr("Unable to create SQLite database file.");
                return db;
            }
        }

        db.setDatabaseName(dbDir.filePath(QSL("qmanga.sqlite")));

    } else {
        return db;
    }

    if (!db.open()) {
        db = QSqlDatabase();
        Q_EMIT errorMsg(tr("Unable to open database connection. Check connection info.\n%1\n%2")
                      .arg(db.lastError().driverText(),db.lastError().databaseText()));
        m_problems[tr("Connection")] = tr("Unable to connect to database.\n"
                                        "Check credentials and SQL server running.");
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
    QSqlQuery qr(QSL("SELECT filename FROM ignored_files"),db);
    while (qr.next())
        sl << qr.value(0).toString();

    m_ignoredFiles = sl;
}

void ZDB::sqlGetAlbums()
{
    ZAlbumVector result;
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    QSqlQuery qr(QSL("SELECT id, parent, name FROM `albums` ORDER BY name ASC"),db);
    while (qr.next()) {
        bool ok1 = false;
        int id = qr.value(0).toInt(&ok1);
        bool ok2 = false;
        int parent = qr.value(1).toInt(&ok2);
        if (ok1 && ok2)
            result << ZAlbumEntry(id,parent,qr.value(2).toString());
    }

    sqlUpdateIgnoredFiles(db);

    sqlCloseBase(db);

    int id = -1;

    for (auto it = m_dynAlbums.constKeyValueBegin(), end = m_dynAlbums.constKeyValueEnd();
         it != end; ++it)
        result << ZAlbumEntry(id--,dynamicAlbumParent,QSL("# %1").arg((*it).first));

    result << ZAlbumEntry(id--,-1,QSL("% Deleted"));

    Q_EMIT gotAlbums(result);
}

void ZDB::sqlGetFiles(const QString &album, const QString &search)
{
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    QElapsedTimer tmr;
    tmr.start();

    constexpr int fldName = 0;
    constexpr int fldPagesCount = 3;
    constexpr int fldFileSize = 4;
    constexpr int fldFileMagic = 5;
    constexpr int fldFileDate = 6;
    constexpr int fldFileAdded = 7;
    constexpr int fldID = 8;
    constexpr int fldAlbumName = 9;
    constexpr int fldPreferredRendering = 10;
    QString tqr = QSL("SELECT files.name, filename, cover, pagesCount, fileSize, "
                                 "fileMagic, fileDT, addingDT, files.id, albums.name, "
                                 "files.preferredRendering "
                                 "FROM files LEFT JOIN albums ON files.album=albums.id ");

    bool checkFS = false;

    bool albumBind = false;
    bool searchBind = false;

    if (!album.isEmpty()) {
        if (album.startsWith(QSL("# "))) {
            QString name = album.mid(2);
            if (m_dynAlbums.contains(name)){
                tqr += m_dynAlbums.value(name);
            }
        } else if (album.startsWith(QSL("% Deleted"))) {
            checkFS = true;
        } else {
            tqr += QSL("WHERE (album="
                           "  (SELECT id FROM albums WHERE (name = ?))"
                           ") ");
            albumBind = true;
        }
    } else if (!search.isEmpty()) {
        QString sqr;
        if (sqlDbEngine(db)==Z::dbmsMySQL)
            sqr = prepareSearchQuery(search);
        if (sqr.isEmpty()) {
            sqr = QSL("WHERE (files.name LIKE ?) ");
            searchBind = true;
        }
        tqr += sqr;
    }

    int idx = 0;

    QSqlQuery qr(db);
    qr.prepare(tqr);
    if (albumBind)
        qr.addBindValue(album);
    if (searchBind)
        qr.addBindValue(QSL("%%%1%%").arg(search));

    if (qr.exec()) {
        m_preferredRendering.clear();
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

            int prefRendering = qr.value(fldPreferredRendering).toInt();
            m_preferredRendering[fileName] = prefRendering;

            Q_EMIT gotFile(ZSQLMangaEntry(qr.value(fldName).toString(),
                                       fileName,
                                       qr.value(fldAlbumName).toString(),
                                       p,
                                       qr.value(fldPagesCount).toInt(),
                                       qr.value(fldFileSize).toInt(),
                                       qr.value(fldFileMagic).toString(),
                                       qr.value(fldFileDate).toDateTime(),
                                       qr.value(fldFileAdded).toDateTime(),
                                       qr.value(fldID).toInt(),
                                       static_cast<Z::PDFRendering>(prefRendering)));
            QApplication::processEvents();
            idx++;
        }
    } else
        qWarning() << qr.lastError().databaseText() << qr.lastError().driverText();
    sqlCloseBase(db);
    Q_EMIT filesLoaded(idx,tmr.elapsed());
}

void ZDB::sqlChangeFilePreview(const QString &fileName, int pageNum)
{
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    QString fname(fileName);
    bool dynManga = fname.startsWith(QSL("#DYN#"));
    if (dynManga)
        fname.remove(QRegularExpression(QSL("^#DYN#")));

    QSqlQuery qr(db);
    qr.prepare(QSL("SELECT name FROM files WHERE (filename=?)"));
    qr.addBindValue(fname);
    if (!qr.exec()) {
        qWarning() << "file search query failed";
        sqlCloseBase(db);
        return;
    }
    if (!qr.next()) {
        Q_EMIT errorMsg(tr("Opened file not found in library."));
        sqlCloseBase(db);
        return;
    }

    QFileInfo fi(fname);
    if (!fi.isReadable()) {
        qWarning() << "skipping" << fname << "as unreadable";
        Q_EMIT errorMsg(tr("%1 file is unreadable.").arg(fname));
        sqlCloseBase(db);
        return;
    }

    bool mimeOk = false;
    ZAbstractReader* za = zF->readerFactory(this,fileName,&mimeOk,true);
    if (za == nullptr) {
        qWarning() << fname << "File format not supported";
        Q_EMIT errorMsg(tr("%1 file format not supported.").arg(fname));
        sqlCloseBase(db);
        return;
    }

    if (!za->openFile()) {
        qWarning() << fname << "Unable to open file.";
        Q_EMIT errorMsg(tr("Unable to open file %1.").arg(fname));
        za->setParent(nullptr);
        delete za;
        sqlCloseBase(db);
        return;
    }

    QByteArray pba = createMangaPreview(za,pageNum);
    qr.prepare(QSL("UPDATE files SET cover=? WHERE (filename=?)"));
    qr.bindValue(0,pba);
    qr.bindValue(1,fname);
    if (!qr.exec()) {
        QString msg = tr("Unable to change cover for '%1'.\n%2\n%3")
                .arg(fname,qr.lastError().databaseText(),qr.lastError().driverText());
        qWarning() << msg;
        Q_EMIT errorMsg(msg);
    }
    pba.clear();
    za->closeFile();
    za->setParent(nullptr);
    delete za;

    sqlCloseBase(db);
}

void ZDB::sqlRescanIndexedDirs()
{
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    QStringList dirs;

    QSqlQuery qr(QSL("SELECT filename "
                 "FROM files "
                 "WHERE NOT(fileMagic='DYN')"),db);
    while (qr.next()) {
        QFileInfo fi(qr.value(0).toString());
        if (!fi.exists()) continue;
        if (!dirs.contains(fi.absoluteDir().absolutePath()))
            dirs << fi.absoluteDir().absolutePath();
    }

    sqlCloseBase(db);

    m_indexedDirs.clear();
    if (!dirs.isEmpty()) {
        m_indexedDirs.append(dirs);
        Q_EMIT updateWatchDirList(dirs);
    }
    Q_EMIT albumsListUpdated();
}

void ZDB::sqlUpdateFileStats(const QString &fileName)
{
    bool dynManga = false;
    QString fname = fileName;
    if (fname.startsWith(QSL("#DYN#"))) {
        fname.remove(QRegularExpression(QSL("^#DYN#")));
        dynManga = true;
    }

    QFileInfo fi(fname);
    if (!fi.isReadable()) {
        qWarning() << "updating aborted for" << fname << "as unreadable";
        return;
    }

    bool mimeOk = false;
    ZAbstractReader* za = zF->readerFactory(this,fileName,&mimeOk,true);
    if (za == nullptr) {
        qWarning() << fname << "File format not supported.";
        return;
    }

    if (!za->openFile()) {
        qWarning() << fname << "Unable to open file.";
        za->setParent(nullptr);
        delete za;
        return;
    }

    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    QSqlQuery qr(db);
    qr.prepare(QSL("UPDATE files SET pagesCount=?, fileSize=?, fileMagic=?, fileDT=? "
               "WHERE (filename=?)"));

    qr.bindValue(0,za->getPageCount());
    if (dynManga) {
        qr.bindValue(1,0);
    } else {
        qr.bindValue(1,fi.size());
    }
    qr.bindValue(2,za->getMagic());
    qr.bindValue(3,fi.birthTime());
    qr.bindValue(4,fname);

    za->closeFile();
    za->setParent(nullptr);
    delete za;

    if (!qr.exec()) {
        qWarning() << fname << "unable to update file stats" <<
                    qr.lastError().databaseText() << qr.lastError().driverText();
    }

    sqlCloseBase(db);
}

void ZDB::sqlSearchMissingManga()
{
    QStringList foundFiles;

    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    QStringList filenames;

    for (const QString& d : qAsConst(m_indexedDirs)) {
        QDir dir(d);
        const QFileInfoList fl = dir.entryInfoList(
                                     QStringList(QSL("*")), QDir::Files | QDir::Readable);
        for (const QFileInfo &fi : fl)
            filenames << fi.absoluteFilePath();
    }

    QSqlQuery qr(db);

    qr.prepare(QSL("DROP TABLE IF EXISTS `tmp_exfiles`"));
    if (!qr.exec()) {
        Q_EMIT errorMsg(tr("Unable to drop table `tmp_exfiles`\n\n%1\n%2")
                      .arg(qr.lastError().databaseText(),qr.lastError().driverText()));
        m_problems[tr("Drop tables - tmp_exfiles")] = tr("Unable to drop temporary table `tmp_exfiles`.\n"
                                                       "DROP TABLE query failed.");
        sqlCloseBase(db);
        return;
    }

    qr.clear();
    if (sqlDbEngine(db)==Z::dbmsMySQL) {
        qr.prepare(QSL("CREATE TEMPORARY TABLE IF NOT EXISTS `tmp_exfiles` ("
                   "`filename` varchar(16383), INDEX ifilename (filename)"
                   ") ENGINE=MyISAM DEFAULT CHARSET=utf8"));
    } else if (sqlDbEngine(db)==Z::dbmsSQLite) {
        qr.prepare(QSL("CREATE TEMPORARY TABLE IF NOT EXISTS "
                                  "`tmp_exfiles` (`filename` TEXT)"));
    }
    if (!qr.exec()) {
        Q_EMIT errorMsg(tr("Unable to create temporary table `tmp_exfiles`\n\n%1\n%2")
                      .arg(qr.lastError().databaseText(),qr.lastError().driverText()));
        m_problems[tr("Create tables - tmp_exfiles")] =
                tr("Unable to create temporary table `tmp_exfiles`.\n"
                   "CREATE TABLE query failed.");
        sqlCloseBase(db);
        return;
    }

    QVariantList sl;
    while (!filenames.isEmpty()) {
        sl.clear();
        sl.reserve(ZDefaults::maxSQLUpdateStringListSize);
        while (sl.count()<ZDefaults::maxSQLUpdateStringListSize && !filenames.isEmpty())
            sl << filenames.takeFirst();
        if (!sl.isEmpty()) {
            qr.prepare(QSL("INSERT INTO `tmp_exfiles` VALUES (?)"));
            qr.addBindValue(sl);
            if (!qr.execBatch()) {
                Q_EMIT errorMsg(tr("Unable to load data to table `tmp_exfiles`\n\n%1\n%2")
                              .arg(qr.lastError().databaseText(),qr.lastError().driverText()));
                m_problems[tr("Load data infile - tmp_exfiles")] =
                        tr("Unable to load data to temporary table `tmp_exfiles`.\n"
                           "INSERT INTO query failed.");
                sqlCloseBase(db);
                return;
            }
        }
    }

    qr.prepare(QSL("SELECT tmp_exfiles.filename FROM tmp_exfiles "
               "LEFT JOIN files ON tmp_exfiles.filename=files.filename "
               "LEFT JOIN ignored_files ON tmp_exfiles.filename=ignored_files.filename "
               "WHERE (files.filename IS NULL) AND (ignored_files.filename IS NULL)"));
    if (qr.exec()) {
        while (qr.next())
            foundFiles << qr.value(0).toString();
    } else
        qWarning() << qr.lastError().databaseText() << qr.lastError().driverText();

    qr.prepare(QSL("DROP TABLE `tmp_exfiles`"));
    if (!qr.exec())
        qWarning() << qr.lastError().databaseText() << qr.lastError().driverText();

    sqlCloseBase(db);

    Q_EMIT foundNewFiles(foundFiles);
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
    if (!sqlHaveTables(db)) {
        sqlCloseBase(db);
        return;
    }

    QSqlQuery qr(db);

    if (cleanTable) {
        qr.prepare(QSL("DELETE FROM `ignored_files`"));
        if (!qr.exec()) {
            Q_EMIT errorMsg(tr("Unable to delete from table `ignored_files`\n\n%1\n%2")
                          .arg(qr.lastError().databaseText(),qr.lastError().driverText()));
            m_problems[tr("Delete from table - ignored_files")] =
                    tr("Unable to delete from table `ignored_files`.\n"
                       "DELETE FROM query failed.");
            sqlCloseBase(db);
            return;
        }
        m_ignoredFiles.clear();
    }

    for (const QString& file : files) {
        qr.prepare(QSL("INSERT INTO ignored_files (filename) VALUES (?)"));
        qr.addBindValue(file);
        if (!qr.exec()) {
            Q_EMIT errorMsg(tr("Unable to add ignored file `%1`\n%2\n%3").
                          arg(file,qr.lastError().databaseText(),qr.lastError().driverText()));
            sqlCloseBase(db);
            return;
        }
        m_ignoredFiles << file;
    }
    sqlCloseBase(db);
}

Z::DBMS ZDB::sqlDbEngine(QSqlDatabase &db)
{
    if (db.isValid() && db.driver()!=nullptr) {
        if (db.driver()->dbmsType()==QSqlDriver::MySqlServer) return Z::dbmsMySQL;
        if (db.driver()->dbmsType()==QSqlDriver::SQLite) return Z::dbmsSQLite;
    }
    return Z::dbmsUndefinedDB;
}

bool ZDB::sqlHaveTables(QSqlDatabase &db)
{
    return sqlCheckBasePriv(db,true);
}

int ZDB::sqlFindAndAddAlbum(const QString &name, const QString &parent, bool createNew)
{
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return -1;
    QSqlQuery qr(db);

    qr.prepare(QSL("SELECT id FROM albums WHERE (name=?)"));
    qr.addBindValue(name);
    if (qr.exec()) {
        if (qr.next()) {
            sqlCloseBase(db);
            return qr.value(0).toInt();
        }
    }

    if (!createNew)
        return -1;

    int idParent = -1;
    if (!parent.isEmpty()) {
        qr.prepare(QSL("SELECT id FROM albums WHERE (name=?)"));
        qr.addBindValue(parent);
        if (qr.exec()) {
            if (qr.next())
                idParent = qr.value(0).toInt();
        }
    }

    qr.prepare(QSL("INSERT INTO albums (name,parent) VALUES (?,?)"));
    qr.bindValue(0,name);
    qr.bindValue(1,idParent);
    if (!qr.exec()) {
        Q_EMIT errorMsg(tr("Unable to create album `%1`\n%2\n%3").
                      arg(name,qr.lastError().databaseText(),qr.lastError().driverText()));
        sqlCloseBase(db);
        return -1;
    }

    bool ok = false;
    int ialbum = qr.lastInsertId().toInt(&ok);
    if (!ok) {
        ialbum = -1;
        qr.prepare(QSL("SELECT id FROM albums WHERE (name=?)"));
        qr.addBindValue(name);
        if (qr.exec()) {
            if (qr.next())
                ialbum = qr.value(0).toInt();
        }
    }

    sqlCloseBase(db);
    return ialbum;
}

QStringList ZDB::sqlGetIgnoredFiles() const
{
    return m_ignoredFiles;
}

Z::PDFRendering ZDB::getPreferredRendering(const QString &filename) const
{
    if (m_preferredRendering.contains(filename))
        return static_cast<Z::PDFRendering>(m_preferredRendering.value(filename));

    return Z::PDFRendering::pdfAutodetect;
}

void ZDB::sqlSetPreferredRendering(const QString &filename, int mode)
{
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    QSqlQuery qr(db);
    qr.prepare(QSL("UPDATE files SET preferredRendering=? WHERE filename=?"));
    qr.bindValue(0,mode);
    qr.bindValue(1,filename);
    if (!qr.exec()) {
        QString msg = tr("Unable to change preferred rendering for '%1'.\n%2\n%3").
                      arg(filename,qr.lastError().databaseText(),qr.lastError().driverText());
        qWarning() << msg;
        Q_EMIT errorMsg(msg);
    }
    m_preferredRendering[filename] = mode;
    sqlCloseBase(db);
}

void ZDB::sqlGetTablesDescription()
{
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;
    if (db.driver()==nullptr) {
        sqlCloseBase(db);
        return;
    }

    QSqlRecord rec = db.driver()->record(QSL("files"));
    if (rec.isEmpty()) {
        sqlCloseBase(db);
        return;
    }

    QStringList names;
    QStringList types;
    names.reserve(rec.count());
    int nl = 0; int tl = 0;
    for (int i=0; i < rec.count(); i++)
    {
        names << rec.field(i).name();
        switch (rec.field(i).type()) {
            case QVariant::Int:
            case QVariant::UInt:
            case QVariant::ULongLong:
            case QVariant::LongLong:
                types << QSL("NUMERIC");
                break;
            case QVariant::String:
            case QVariant::StringList:
                types << QSL("STRING");
                break;
            case QVariant::Icon:
            case QVariant::Image:
            case QVariant::Bitmap:
            case QVariant::Pixmap:
            case QVariant::ByteArray:
                types << QSL("BLOB");
                break;
            case QVariant::Date:
                types << QSL("DATE");
                break;
            case QVariant::DateTime:
                types << QSL("DATETIME");
                break;
            case QVariant::Time:
                types << QSL("TIME");
                break;
            case QVariant::Double:
                types << QSL("DOUBLE");
                break;
            default:
                types << QSL("unknown");
                break;
        }
        if (names.last().length()>nl)
            nl = names.last().length();

        if (types.last().length()>tl)
            tl = types.last().length();
    }

    sqlCloseBase(db);

    QStringList res;
    QString s1(QSL("Field"));
    QString s2(QSL("Type"));
    res << QSL("Fields description:");
    res << QSL("| ")+s1.leftJustified(nl,' ')+QSL(" | ")
           +s2.leftJustified(tl,' ')+QSL(" |");
    s1.fill('-',nl); s2.fill('-',tl);
    res << QSL("+-")+s1+QSL("-+-")+s2+QSL("-+");
    res.reserve(names.count());
    for (int i=0;i<names.count();i++) {
        s1 = names.at(i);
        s2 = types.at(i);
        res << QSL("| ")+s1.leftJustified(nl,' ')
               +QSL(" | ")+s2.leftJustified(tl,' ')+QSL(" |");
    }
    Q_EMIT gotTablesDescription(res.join('\n'));
}

void ZDB::sqlAddFiles(const QStringList& aFiles, const QString& album)
{
    if (album.isEmpty()) {
        Q_EMIT errorMsg(tr("Album name cannot be empty"));
        return;
    }
    if (album.startsWith('#') || album.startsWith('%')) {
        Q_EMIT errorMsg(tr("Reserved character in album name"));
        return;
    }
    if (aFiles.first().startsWith(QSL("###DYNAMIC###"))) {
        fsAddImagesDir(aFiles.last(),album);
        return;
    }

    QStringList files = aFiles;
    files.removeDuplicates();

    int ialbum = sqlFindAndAddAlbum(album);
    if (ialbum<0) {
        qWarning() << "Album '" << album << "' not found, unable to create album.";
        return;
    }

    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;
    QSqlQuery qr(db);

    m_wasCanceled = false;

    QElapsedTimer tmr;
    tmr.start();

    Q_EMIT showProgressDialog(true);

    int cnt = 0;
    for (int i=0;i<files.count();i++) {
        QFileInfo fi(files.at(i));
        if (!fi.isReadable()) {
            qWarning() << "skipping" << files.at(i) << "as unreadable";
            continue;
        }

        bool mimeOk = false;
        ZAbstractReader* za = zF->readerFactory(this,files.at(i),&mimeOk,true);
        if (za == nullptr) {
            qWarning() << files.at(i) << "File format not supported.";
            continue;
        }

        if (!za->openFile()) {
            qWarning() << files.at(i) << "Unable to open file.";
            za->setParent(nullptr);
            delete za;
            continue;
        }

        QByteArray pba = createMangaPreview(za,0);

        constexpr int fldName = 0;
        constexpr int fldFilename = 1;
        constexpr int fldAlbum = 2;
        constexpr int fldCover = 3;
        constexpr int fldPagesCount = 4;
        constexpr int fldFileSize = 5;
        constexpr int fldFileMagic = 6;
        constexpr int fldFileDate = 7;
        constexpr int fldFileAdded = 8;
        qr.prepare(QSL("INSERT INTO files (name, filename, album, cover, "
                                  "pagesCount, fileSize, fileMagic, fileDT, addingDT) "
                                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"));
        qr.bindValue(fldName,fi.completeBaseName());
        qr.bindValue(fldFilename,files.at(i));
        qr.bindValue(fldAlbum,ialbum);
        qr.bindValue(fldCover,pba);
        qr.bindValue(fldPagesCount,za->getPageCount());
        qr.bindValue(fldFileSize,fi.size());
        qr.bindValue(fldFileMagic,za->getMagic());
        qr.bindValue(fldFileDate,fi.birthTime());
        qr.bindValue(fldFileAdded,QDateTime::currentDateTime());
        if (!qr.exec()) {
            qWarning() << files.at(i) << "unable to add" <<
                        qr.lastError().databaseText() << qr.lastError().driverText();
        } else {
            cnt++;
        }
        pba.clear();
        za->closeFile();
        za->setParent(nullptr);
        delete za;

        QString s = fi.fileName();
        if (s.length()>ZDefaults::maxDescriptionStringLength) {
            s.truncate(ZDefaults::maxDescriptionStringLength);
            s.append(QSL("..."));
        }
        Q_EMIT showProgressState(100*i/files.count(),s);
        QApplication::processEvents();

        if (m_wasCanceled) {
            m_wasCanceled = false;
            break;
        }
        QApplication::processEvents();
    }
    Q_EMIT showProgressDialog(false);
    sqlCloseBase(db);
    sqlRescanIndexedDirs();
    Q_EMIT filesAdded(cnt,files.count(),tmr.elapsed());
    Q_EMIT albumsListUpdated();
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
        p = p.scaled(ZDefaults::maxPreviewSize,ZDefaults::maxPreviewSize,
                     Qt::KeepAspectRatio,Qt::SmoothTransformation);
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
        Q_EMIT errorMsg(QSL("Updating aborted for %1 as unreadable").arg(dir));
        return;
    }

    QDir d(dir);
    const QFileInfoList fl = zF->filterSupportedImgFiles(d.entryInfoList(QStringList(QSL("*")),
                                                                         QDir::Files | QDir::Readable));
    QStringList files;
    files.reserve(fl.count());
    for (const QFileInfo &fi : fl)
        files << fi.absoluteFilePath();

    if (files.isEmpty()) {
        Q_EMIT errorMsg(tr("Could not find supported image files in specified directory"));
        return;
    }

    // get album number
    int ialbum = sqlFindAndAddAlbum(album);
    if (ialbum<0) {
        qWarning() << "Album '" << album << "' not found, unable to create album.";
        return;
    }

    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;
    QSqlQuery qr(db);

    QElapsedTimer tmr;
    tmr.start();

    // get album preview image
    QImage p;
    for (const QString& fname : qAsConst(files)) {
        if (p.load(fname))
            break;
    }

    QByteArray pba;
    pba.clear();
    if (!p.isNull()) {
        p = p.scaled(ZDefaults::maxPreviewSize,ZDefaults::maxPreviewSize,
                     Qt::KeepAspectRatio,Qt::SmoothTransformation);
        QBuffer buf(&pba);
        buf.open(QIODevice::WriteOnly);
        p.save(&buf, "JPG");
        buf.close();
        p = QImage();
    }

    // add dynamic album to base
    int cnt = 0;
    constexpr int fldName = 0;
    constexpr int fldFilename = 1;
    constexpr int fldAlbum = 2;
    constexpr int fldCover = 3;
    constexpr int fldPagesCount = 4;
    constexpr int fldFileSize = 5;
    constexpr int fldFileMagic = 6;
    constexpr int fldFileDate = 7;
    constexpr int fldFileAdded = 8;
    qr.prepare(QSL("INSERT INTO files (name, filename, album, cover, pagesCount, "
                              "fileSize, fileMagic, fileDT, addingDT) VALUES "
                              "(?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    qr.bindValue(fldName,d.dirName());
    qr.bindValue(fldFilename,d.absolutePath());
    qr.bindValue(fldAlbum,ialbum);
    qr.bindValue(fldCover,pba);
    qr.bindValue(fldPagesCount,files.count());
    qr.bindValue(fldFileSize,0);
    qr.bindValue(fldFileMagic,QSL("DYN"));
    qr.bindValue(fldFileDate,fi.birthTime());
    qr.bindValue(fldFileAdded,QDateTime::currentDateTime());
    if (!qr.exec()) {
        qWarning() << "unable to add dynamic album" << dir <<
                      qr.lastError().databaseText() << qr.lastError().driverText();
    } else {
        cnt++;
    }
    pba.clear();

    sqlCloseBase(db);
    sqlRescanIndexedDirs();
    Q_EMIT filesAdded(cnt,cnt,tmr.elapsed());
    Q_EMIT albumsListUpdated();
}

QString ZDB::prepareSearchQuery(const QString &search)
{
    static const QStringList operators({QSL(" +"), QSL(" ~"), QSL(" -"), QSL("* "), QSL("\"")});
    QString msearch = search;
    if (search.contains(QRegularExpression(QSL("\\s")))) {
        bool haveops = false;
        for (const auto &i : operators) {
            if (msearch.contains(i)) {
                haveops = true;
                break;
            }
        }
        if (!haveops) {
            msearch.replace(QRegularExpression(QSL("\\s+")),QSL("* +"));
            msearch.prepend(QChar('+'));
            msearch.append(QChar('*'));
        }
        return QSL("WHERE MATCH(files.name) AGAINST('%1' IN BOOLEAN MODE) ")
                .arg(zF->escapeParam(msearch));
    }
    return QString();
}

void ZDB::sqlCancelAdding()
{
    m_wasCanceled = true;
}

void ZDB::sqlDelFiles(const ZIntVector &dbids, bool fullDelete)
{
    if (dbids.count()==0) return;

    QString delqr = QSL("DELETE FROM files WHERE id IN (");
    for (int i=0;i<dbids.count();i++) {
        if (i>0) delqr+=QSL(",");
        delqr+=QSL("%1").arg(dbids.at(i));
    }
    delqr+=QSL(")");

    QString fsqr = QSL("SELECT filename,fileMagic FROM files WHERE id IN (");
    for (int i=0;i<dbids.count();i++) {
        if (i>0) fsqr+=QSL(",");
        fsqr+=QSL("%1").arg(dbids.at(i));
    }
    fsqr+=QSL(")");

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
                if (magic!=QSL("DYN")) {
                    if (!QFile::remove(fname))
                        okdel = false;
                }
            }
        }
    }

    qr.prepare(delqr);
    if (!qr.exec()) {
        Q_EMIT errorMsg(tr("Unable to delete manga\n%1\n%2").
                      arg(qr.lastError().databaseText(), qr.lastError().driverText()));
        sqlCloseBase(db);
        return;
    }
    sqlCloseBase(db);
    sqlRescanIndexedDirs();

    Q_EMIT deleteItemsFromModel(dbids);

    if (!okdel && fullDelete) {
        Q_EMIT errorMsg(tr("Errors occured, some files not deleted from filesystem or not found.\n%1\n%2").
                      arg(db.lastError().databaseText(), db.lastError().driverText()));
    }
}

void ZDB::sqlRenameAlbum(const QString &oldName, const QString &newName)
{
    if (oldName==newName || oldName.isEmpty() || newName.isEmpty()) return;
    if (newName.startsWith('#') || oldName.startsWith('#')) return;
    if (newName.startsWith('%') || oldName.startsWith('%')) return;

    if (sqlFindAndAddAlbum(newName,QString(),false)>=0) {
        qWarning() << "Unable to rename album - name already exists.";
        Q_EMIT errorMsg(tr("Unable to rename album - name already exists."));
        return;
    }

    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    QSqlQuery qr(db);
    qr.prepare(QSL("UPDATE albums SET name=? WHERE name=?"));
    qr.bindValue(0,newName);
    qr.bindValue(1,oldName);
    if (!qr.exec()) {
        qWarning() << "Unable to rename album"
                 << qr.lastError().databaseText() << qr.lastError().driverText();
        Q_EMIT errorMsg(tr("Unable to rename album `%1`\n%2\n%3").
                      arg(oldName,qr.lastError().databaseText(), qr.lastError().driverText()));
    }
    sqlCloseBase(db);
    Q_EMIT albumsListUpdated();
}

void ZDB::sqlDelAlbum(const QString &album)
{
    int id = sqlFindAndAddAlbum(album,QString(),false);
    if (id==-1) {
        Q_EMIT errorMsg(tr("Album '%1' not found").arg(album));
        return;
    }

    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    QSqlQuery qr(db);

    qr.prepare(QSL("DELETE FROM files WHERE album=?"));
    qr.addBindValue(id);
    if (!qr.exec()) {
        QString msg = tr("Unable to delete manga\n%1\n%2").
                      arg(qr.lastError().databaseText(), qr.lastError().driverText());
        Q_EMIT errorMsg(msg);
        qWarning() << msg;
        sqlCloseBase(db);
        return;
    }

    qr.prepare(QSL("UPDATE albums SET parent=-1 WHERE parent=?"));
    qr.addBindValue(id);
    if (!qr.exec()) {
        QString msg = tr("Unable to unlink album as parent\n%1\n%2").
                      arg(qr.lastError().databaseText(), qr.lastError().driverText());
        Q_EMIT errorMsg(msg);
        qWarning() << msg;
        sqlCloseBase(db);
        return;
    }

    qr.prepare(QSL("DELETE FROM albums WHERE id=?"));
    qr.addBindValue(id);
    if (!qr.exec()) {
        QString msg = tr("Unable to delete album\n%1\n%2").
                      arg(qr.lastError().databaseText(), qr.lastError().driverText());
        Q_EMIT errorMsg(msg);
        qWarning() << msg;
        sqlCloseBase(db);
        return;
    }

    sqlCloseBase(db);
    sqlRescanIndexedDirs();
}

void ZDB::sqlAddAlbum(const QString &album, const QString &parent)
{
    int id = sqlFindAndAddAlbum(album,parent);
    if (id<0) {
        QString msg = tr("Unable to create album %1").arg(album);
        Q_EMIT errorMsg(msg);
        qWarning() << msg;
    } else
        Q_EMIT albumsListUpdated();
}

void ZDB::sqlReparentAlbum(const QString &album, const QString &parent)
{
    int srcId = sqlFindAndAddAlbum(album,QString(),false);
    if (srcId<0) {
        QString msg = tr("Unable to find album %1").arg(album);
        Q_EMIT errorMsg(msg);
        qWarning() << msg;
        return;
    }
    int parentId = -1;
    if (!parent.isEmpty()) {
        parentId = sqlFindAndAddAlbum(parent,QString(),false);
        if (parentId<0) {
            QString msg = tr("Unable to find parent album %1").arg(parent);
            Q_EMIT errorMsg(msg);
            qWarning() << msg;
            return;
        }
    }

    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    QSqlQuery qr(db);

    qr.prepare(QSL("UPDATE albums SET parent=? WHERE id=?"));
    qr.bindValue(0,parentId);
    qr.bindValue(1,srcId);
    if (!qr.exec()) {
        QString msg = tr("Unable to set album's parent\n%1\n%2").
                      arg(qr.lastError().databaseText(), qr.lastError().driverText());
        Q_EMIT errorMsg(msg);
        qWarning() << msg;
        sqlCloseBase(db);
        return;
    }

    Q_EMIT albumsListUpdated();
}
