#include <utility>
#include <algorithm>
#include <execution>

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
#include "global.h"

ZDB::ZDB(QObject *parent) :
    QObject(parent)
{
    m_dbHost = QSL("localhost");
    m_dbBase = QSL("qmanga");
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

bool ZDB::isDynamicAlbumParent(int parentId)
{
    return (parentId == dynamicAlbumParent);
}

int ZDB::getDynamicAlbumParent()
{
    return dynamicAlbumParent;
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

void ZDB::setCoverCacheSize(int size)
{
    QMutexLocker locker(&m_coverCacheMutex);

    if (m_coverCache.maxCost() != size)
        m_coverCache.setMaxCost(size);
}

void ZDB::setDynAlbums(const ZStrMap &albums)
{
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

QSqlDatabase ZDB::sqlOpenBase(bool silent)
{
    QSqlDatabase db;

    if (zF->global()==nullptr) return db;

    if (zF->global()->getDbEngine()==Z::dbmsMySQL) {
        db = QSqlDatabase::addDatabase(QSL("QMYSQL"),QUuid::createUuid().toString());
        if (!db.isValid()) {
            db = QSqlDatabase();
            if (!silent) {
                Q_EMIT errorMsg(tr("Unable to create MySQL driver instance."));
                m_problems[tr("Connection")] = tr("Unable to create MySQL driver instance.");
            }
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
            if (!silent) {
                Q_EMIT errorMsg(tr("Unable to create SQLite driver instance."));
                m_problems[tr("Connection")] = tr("Unable to create SQLite driver instance.");
            }
            return db;
        }

        QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dbDir(dir);
        if (!dbDir.exists()) {
            if (!dbDir.mkpath(dir)) {
                db = QSqlDatabase();
                if (!silent) {
                    Q_EMIT errorMsg(tr("Unable to create SQLite database file. Check file permissions.\n%1")
                                    .arg(dir));
                    m_problems[tr("Connection")] = tr("Unable to create SQLite database file.");
                }
                return db;
            }
        }

        db.setDatabaseName(dbDir.filePath(QSL("qmanga.sqlite")));

    } else {
        return db;
    }

    if (!db.open()) {
        db = QSqlDatabase();
        if (!silent) {
            Q_EMIT errorMsg(tr("Unable to open database connection. Check connection info.\n%1\n%2")
                            .arg(db.lastError().driverText(),db.lastError().databaseText()));
            m_problems[tr("Connection")] = tr("Unable to connect to database.\n"
                                        "Check credentials and SQL server running.");
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

void ZDB::sqlGetFiles(const QString &album, const QString &search, const QSize& preferredCoverSize)
{
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    QElapsedTimer tmr;
    tmr.start();

    const int fldName = 0;
    const int fldPagesCount = 3;
    const int fldFileSize = 4;
    const int fldFileMagic = 5;
    const int fldFileDate = 6;
    const int fldFileAdded = 7;
    const int fldID = 8;
    const int fldAlbumName = 9;
    const int fldPreferredRendering = 10;
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
            if (!ba.isEmpty()) {
                if (ba.startsWith(ZDefaults::coverBase64Header))
                    ba = QByteArray::fromBase64(ba.mid(ZDefaults::coverBase64Header.size()));
                p.loadFromData(ba);
            }

            QString fileName = qr.value(1).toString();

            if (checkFS) {
                QFileInfo fi(fileName);
                if (fi.exists()) continue;
            }

            int prefRendering = qr.value(fldPreferredRendering).toInt();
            m_preferredRendering[fileName] = prefRendering;

            idx++;
            const ZSQLMangaEntry entry(ZSQLMangaEntry(qr.value(fldName).toString(),
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

            {
                QMutexLocker mlock(&m_coverCacheMutex);

                const auto *cached = m_coverCache.object(entry.filename);
                if (cached && (cached->first == preferredCoverSize)) {
                    const QImage res(cached->second);
                    ZSQLMangaEntry e = entry;
                    if (!res.isNull()) {
                        e.cover = res;

                        Q_EMIT gotFile(e);
                        continue;
                    }
                }
            }

            m_fastResamplersPool.start([this,entry,preferredCoverSize](){
                const QImage res = entry.cover.scaled(preferredCoverSize,Qt::KeepAspectRatio,
                                                      Qt::FastTransformation);
                ZSQLMangaEntry e = entry;
                if (!res.isNull())
                    e.cover = res;

                Q_EMIT gotFile(e);

                m_searchResamplersPool.start([this,entry,preferredCoverSize](){
                    const QImage res = ZGenericFuncs::resizeImage(entry.cover,preferredCoverSize,true,
                                                                  zF->global()->getDownscaleSearchTabFilter());
                    if (!res.isNull()) {
                        QMutexLocker mlock(&m_coverCacheMutex);
                        m_coverCache.insert(entry.filename,
                                            new QPair<QSize,QImage>(preferredCoverSize, res),
                                            res.sizeInBytes());
                        QMetaObject::invokeMethod(this,[this,entry,res](){
                            Q_EMIT gotResampledCover(entry.dbid,res);
                        },Qt::QueuedConnection);
                    }
                });
            });
        }
    } else {
        qWarning() << qr.lastError().databaseText() << qr.lastError().driverText();
    }
    sqlCloseBase(db);
    m_fastResamplersPool.waitForDone();
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
    ZAbstractReader* za = ZGenericFuncs::readerFactory(this,fileName,&mimeOk,Z::rffSkipSingleImageReader,
                                                       Z::rfmCreateReader);
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

    const QByteArray pba = createMangaPreview(za,pageNum);
    qr.prepare(QSL("UPDATE files SET cover=? WHERE (filename=?)"));
    qr.bindValue(0,pba);
    qr.bindValue(1,fname);
    if (!qr.exec()) {
        QString msg = tr("Unable to change cover for '%1'.\n%2\n%3")
                .arg(fname,qr.lastError().databaseText(),qr.lastError().driverText());
        qWarning() << msg;
        Q_EMIT errorMsg(msg);
    }
    za->closeFile();
    za->setParent(nullptr);
    delete za;

    sqlCloseBase(db);

    QMutexLocker mlock(&m_coverCacheMutex);
    m_coverCache.remove(fname);
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
    ZAbstractReader* za = ZGenericFuncs::readerFactory(this,fileName,&mimeOk,Z::rffSkipSingleImageReader,
                                                       Z::rfmCreateReader);
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
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    QStringList ignoredFiles;
    QSqlQuery qr(QSL("SELECT filename FROM ignored_files"),db);
    while (qr.next())
        ignoredFiles.append(qr.value(0).toString());

    QStringList filenames;
    for (const QString& d : std::as_const(m_indexedDirs)) {
        QDir dir(d);
        const QFileInfoList fl = dir.entryInfoList(
                                     QStringList(QSL("*")), QDir::Files | QDir::Readable);
        for (const QFileInfo &fi : fl) {
            const QString fname = fi.absoluteFilePath();
            if (!ignoredFiles.contains(fname))
                filenames.append(fname);
        }
    }

    QStringList indexedFiles;
    QSqlQuery qr2(QSL("SELECT filename "
                     "FROM files "
                     "WHERE NOT(fileMagic='DYN')"),
                 db);
    while (qr2.next())
        indexedFiles.append(qr2.value(0).toString());

    sqlCloseBase(db);

    std::sort(std::execution::par, filenames.begin(), filenames.end());
    std::sort(std::execution::par, indexedFiles.begin(), indexedFiles.end());

    QStringList foundFiles;
    std::set_difference(filenames.constBegin(),
                        filenames.constEnd(),
                        indexedFiles.constBegin(),
                        indexedFiles.constEnd(),
                        std::inserter(foundFiles, foundFiles.begin()));

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
        names.append(rec.field(i).name());
        switch (rec.field(i).metaType().id()) {
            case QMetaType::Int:
            case QMetaType::UInt:
            case QMetaType::ULongLong:
            case QMetaType::LongLong:
                types.append(QSL("NUMERIC"));
                break;
            case QMetaType::QString:
            case QMetaType::QStringList:
                types.append(QSL("STRING"));
                break;
            case QMetaType::QIcon:
            case QMetaType::QImage:
            case QMetaType::QBitmap:
            case QMetaType::QPixmap:
            case QMetaType::QByteArray:
                types.append(QSL("BLOB"));
                break;
            case QMetaType::QDate:
                types.append(QSL("DATE"));
                break;
            case QMetaType::QDateTime:
                types.append(QSL("DATETIME"));
                break;
            case QMetaType::QTime:
                types.append(QSL("TIME"));
                break;
            case QMetaType::Double:
                types.append(QSL("DOUBLE"));
                break;
            default:
                types.append(QSL("unknown"));
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
    res.append(QSL("Fields description:"));
    res.append(QSL("| ")+s1.leftJustified(nl,u' ')+QSL(" | ")
               +s2.leftJustified(tl,u' ')+QSL(" |"));
    s1.fill(u'-',nl); s2.fill(u'-',tl);
    res.append(QSL("+-")+s1+QSL("-+-")+s2+QSL("-+"));
    res.reserve(names.count());
    for (int i=0;i<names.count();i++) {
        s1 = names.at(i);
        s2 = types.at(i);
        res.append(QSL("| ")+s1.leftJustified(nl,u' ')
                   +QSL(" | ")+s2.leftJustified(tl,u' ')+QSL(" |"));
    }
    Q_EMIT gotTablesDescription(res.join(u'\n'));
}

void ZDB::sqlAddFiles(const QStringList& aFiles, const QString& album)
{
    const int threadPoolWaitTimeoutMS = 250;

    if (album.isEmpty()) {
        Q_EMIT errorMsg(tr("Album name cannot be empty"));
        return;
    }
    if (album.startsWith(u'#') || album.startsWith(u'%')) {
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

    m_wasCanceled = false;

    QElapsedTimer tmr;
    tmr.start();

    Q_EMIT showProgressDialog(true);

    QAtomicInteger<int> addedCount(0);
    QAtomicInteger<bool> stopAdding(false);

    const int totalCount = files.count();

    for (const auto& filename : std::as_const(files)) {
        const QFileInfo fi(filename);
        if (!fi.isReadable()) {
            qWarning() << "Skipping " << filename << " as unreadable ";
            continue;
        }

        if (stopAdding)
            break;

        m_sourceResamplersPool.start([this,fi,&addedCount,&stopAdding,totalCount,ialbum](){
            bool mimeOk = false;

            QScopedPointer<ZAbstractReader> za(ZGenericFuncs::readerFactory(nullptr,fi.filePath(),&mimeOk,
                                                                            Z::rffSkipSingleImageReader,
                                                                            Z::rfmCreateReader));
            if (za.isNull()) {
                qWarning() << "File format not supported: " << fi.filePath();
                return;
            }

            if (!za->openFile()) {
                qWarning() << "Unable to open file: " << fi.filePath();
                return;
            }

            const QByteArray pba = createMangaPreview(za.data(),0);
            const int pageCount = za->getPageCount();
            const QString magic = za->getMagic();

            za->closeFile();

            if (stopAdding)
                return;

            QSqlDatabase db = sqlOpenBase(true);
            if (!db.isValid())
                return;

            QSqlQuery qr(db);

            const int fldName = 0;
            const int fldFilename = 1;
            const int fldAlbum = 2;
            const int fldCover = 3;
            const int fldPagesCount = 4;
            const int fldFileSize = 5;
            const int fldFileMagic = 6;
            const int fldFileDate = 7;
            const int fldFileAdded = 8;
            qr.prepare(QSL("INSERT INTO files (name, filename, album, cover, "
                                          "pagesCount, fileSize, fileMagic, fileDT, addingDT) "
                                          "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"));
            qr.bindValue(fldName,fi.completeBaseName());
            qr.bindValue(fldFilename,fi.filePath());
            qr.bindValue(fldAlbum,ialbum);
            qr.bindValue(fldCover,pba);
            qr.bindValue(fldPagesCount,pageCount);
            qr.bindValue(fldFileSize,fi.size());
            qr.bindValue(fldFileMagic,magic);
            qr.bindValue(fldFileDate,fi.birthTime());
            qr.bindValue(fldFileAdded,QDateTime::currentDateTime());
            if (!qr.exec()) {
                qWarning() << fi.filePath() << " unable to add " <<
                              qr.lastError().databaseText() << qr.lastError().driverText();
            } else {
                addedCount++;
            }
            sqlCloseBase(db);

            QString s = fi.fileName();
            if (s.length()>ZDefaults::maxDescriptionStringLength) {
                s.truncate(ZDefaults::maxDescriptionStringLength);
                s.append(QSL("..."));
            }
            Q_EMIT showProgressState(100*addedCount/totalCount,s);
        });
    }

    while (m_sourceResamplersPool.activeThreadCount() > 0) {
        if (m_wasCanceled) {
            stopAdding = true;
            m_sourceResamplersPool.clear();
            m_wasCanceled = false;
        }
        m_sourceResamplersPool.waitForDone(threadPoolWaitTimeoutMS);
        QApplication::processEvents();
    }

    Q_EMIT showProgressDialog(false);
    sqlRescanIndexedDirs();
    Q_EMIT filesAdded(addedCount,files.count(),tmr.elapsed());
    Q_EMIT albumsListUpdated();
}

QByteArray ZDB::createMangaPreview(ZAbstractReader* za, int pageNum)
{
    QByteArray pba;

    int idx = pageNum;
    QImage p = QImage();
    QByteArray pb;
    while (idx<za->getPageCount()) {
        p = QImage();
        pb = za->loadPage(idx);
        if (!pb.isEmpty()) {
            if (p.loadFromData(pb)) {
                break;
            }
        } else {
            p = za->loadPageImage(idx);
            if (!p.isNull())
                break;
        }
        idx++;
    }
    if (!p.isNull()) {
        p = ZGenericFuncs::resizeImage(p,ZDefaults::maxPreviewSize,true,zF->global()->getDownscaleSearchTabFilter());
        QBuffer buf(&pba);
        buf.open(QIODevice::WriteOnly);
        p.save(&buf, "JPG");
        buf.close();
        p = QImage();
    }
    pba = ZDefaults::coverBase64Header + pba.toBase64();

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
    const QFileInfoList fl = ZGenericFuncs::filterSupportedImgFiles(d.entryInfoList(QStringList(QSL("*")),
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
    for (const QString& fname : std::as_const(files)) {
        if (p.load(fname))
            break;
    }

    QByteArray pba;
    if (!p.isNull()) {
        p = ZGenericFuncs::resizeImage(p,ZDefaults::maxPreviewSize,true,zF->global()->getDownscaleSearchTabFilter());
        QBuffer buf(&pba);
        buf.open(QIODevice::WriteOnly);
        p.save(&buf, "JPG");
        buf.close();
        p = QImage();
    }
    pba = ZDefaults::coverBase64Header + pba.toBase64();

    // add dynamic album to base
    int cnt = 0;
    const int fldName = 0;
    const int fldFilename = 1;
    const int fldAlbum = 2;
    const int fldCover = 3;
    const int fldPagesCount = 4;
    const int fldFileSize = 5;
    const int fldFileMagic = 6;
    const int fldFileDate = 7;
    const int fldFileAdded = 8;
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
            msearch.prepend(QChar(u'+'));
            msearch.append(QChar(u'*'));
        }
        return QSL("WHERE MATCH(files.name) AGAINST('%1' IN BOOLEAN MODE) ")
                .arg(ZGenericFuncs::escapeParam(msearch));
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
    if (newName.startsWith(u'#') || oldName.startsWith(u'#')) return;
    if (newName.startsWith(u'%') || oldName.startsWith(u'%')) return;

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
