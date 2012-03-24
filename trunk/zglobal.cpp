#include "global.h"
#include "zglobal.h"
#include "mainwindow.h"
#include "settingsdialog.h"
#include "zzipreader.h"
#include "zmangamodel.h"
#include <ImageMagick/Magick++.h>

using namespace Magick;

ZGlobal* zGlobal = NULL;

ZGlobal::ZGlobal(QObject *parent) :
    QObject(parent)
{
    cacheWidth = 5;
    bookmarks.clear();
    dbBase = QString("qmanga");
    dbUser = QString();
    dbPass = QString();
}

void ZGlobal::loadSettings()
{
    MainWindow* w = qobject_cast<MainWindow *>(parent());

    QSettings settings("kilobax", "qmanga");
    settings.beginGroup("MainWindow");
    cacheWidth = settings.value("cacheWidth",5).toInt();
    resizeFilter = (ZResizeFilter)settings.value("resizeFilter",0).toInt();
    dbUser = settings.value("mysqlUser",QString()).toString();
    dbPass = settings.value("mysqlPassword",QString()).toString();
    dbBase = settings.value("mysqlBase",QString("qmanga")).toString();
    savedAuxOpenDir = settings.value("savedAuxOpenDir",QString()).toString();
    savedIndexOpenDir = settings.value("savedIndexOpenDir",QString()).toString();
    magnifySize = settings.value("magnifySize",150).toInt();
    backgroundColor = QColor(settings.value("backgroundColor","#303030").toString());
    if (!backgroundColor.isValid())
        backgroundColor = QApplication::palette("QWidget").dark().color();

    bool showMaximized = false;
    if (w!=NULL)
        showMaximized = settings.value("maximized",false).toBool();

    int sz = settings.beginReadArray("bookmarks");
    for (int i=0; i<sz; i++) {
        settings.setArrayIndex(i);
        QString t = settings.value("title").toString();
        if (!t.isEmpty())
            bookmarks[t]=settings.value("url").toString();
    }
    settings.endArray();

    if (w!=NULL) {
        if (settings.value("listMode",false).toBool())
            w->srcWidget->srclModeList->setChecked(true);
        else
            w->srcWidget->srclModeIcon->setChecked(true);
        w->srcWidget->srclIconSize->setValue(settings.value("iconSize",128).toInt());
    }

    settings.endGroup();

    if (w!=NULL) {
        if (showMaximized)
            w->showMaximized();

        w->updateBookmarks();
        w->updateViewer();
        w->srcWidget->setEnabled(!dbUser.isEmpty());
        if (w->srcWidget->isEnabled())
            w->srcWidget->updateAlbumsList();
    }
    sqlCheckBase();
}

void ZGlobal::saveSettings()
{
    QSettings settings("kilobax", "qmanga");
    settings.beginGroup("MainWindow");
    settings.setValue("cacheWidth",cacheWidth);
    settings.setValue("resizeFilter",(int)resizeFilter);
    settings.setValue("mysqlUser",dbUser);
    settings.setValue("mysqlPassword",dbPass);
    settings.setValue("mysqlBase",dbBase);
    settings.setValue("savedAuxOpenDir",savedAuxOpenDir);
    settings.setValue("savedIndexOpenDir",savedIndexOpenDir);
    settings.setValue("magnifySize",magnifySize);
    settings.setValue("backgroundColor",backgroundColor.name());

    MainWindow* w = qobject_cast<MainWindow *>(parent());
    if (w!=NULL) {
        settings.setValue("maximized",w->isMaximized());

        settings.setValue("listMode",w->srcWidget->srclModeList->isChecked());
        settings.setValue("iconSize",w->srcWidget->srclIconSize->value());
    }

    settings.beginWriteArray("bookmarks");
    int i=0;
    foreach (const QString &t, bookmarks.keys()) {
        settings.setArrayIndex(i);
        settings.setValue("title",t);
        settings.setValue("url",bookmarks.value(t));
        i++;
    }
    settings.endArray();

    settings.endGroup();

    sqlDelEmptyAlbums();
}

QString ZGlobal::detectMIME(QString filename)
{
    magic_t myt = magic_open(MAGIC_ERROR|MAGIC_MIME_TYPE);
    magic_load(myt,NULL);
    QByteArray bma = filename.toUtf8();
    const char* bm = bma.data();
    const char* mg = magic_file(myt,bm);
    if (mg==NULL) {
        qDebug() << "libmagic error: " << magic_errno(myt) << QString::fromUtf8(magic_error(myt));
        return QString("text/plain");
    }
    QString mag(mg);
    magic_close(myt);
    return mag;
}

QString ZGlobal::detectMIME(QByteArray buf)
{
    magic_t myt = magic_open(MAGIC_ERROR|MAGIC_MIME_TYPE);
    magic_load(myt,NULL);
    const char* mg = magic_buffer(myt,buf.data(),buf.length());
    if (mg==NULL) {
        qDebug() << "libmagic error: " << magic_errno(myt) << QString::fromUtf8(magic_error(myt));
        return QString("text/plain");
    }
    QString mag(mg);
    magic_close(myt);
    return mag;
}

QPixmap ZGlobal::resizeImage(QPixmap src, QSize targetSize, bool forceFilter, ZResizeFilter filter)
{
    ZResizeFilter rf = resizeFilter;
    if (forceFilter)
        rf = filter;
    if (rf==Nearest)
        return src.scaled(targetSize,Qt::IgnoreAspectRatio,Qt::FastTransformation);
    else if (rf==Bilinear)
        return src.scaled(targetSize,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    else {
        QTime tmr;
        tmr.start();

        // Load QPixmap to ImageMagick image
        QByteArray bufSrc;
        QBuffer buf(&bufSrc);
        buf.open(QIODevice::WriteOnly);
        src.save(&buf,"BMP");
        buf.close();
        Blob iBlob(bufSrc.data(),bufSrc.size());
        Image iImage(iBlob,Geometry(src.width(),src.height()),"BMP");

        // Resize image
        if (rf==Lanczos)
            iImage.filterType(LanczosFilter);
        else if (rf==Gaussian)
            iImage.filterType(GaussianFilter);
        else if (rf==Lanczos2)
            iImage.filterType(Lanczos2Filter);
        else if (rf==Cubic)
            iImage.filterType(CubicFilter);
        else if (rf==Sinc)
            iImage.filterType(SincFilter);
        else if (rf==Triangle)
            iImage.filterType(TriangleFilter);
        else if (rf==Mitchell)
            iImage.filterType(MitchellFilter);
        else
            iImage.filterType(LanczosFilter);

        iImage.resize(Geometry(targetSize.width(),targetSize.height()));

        // Convert image to QPixmap
        Blob oBlob;
        iImage.magick("BMP");
        iImage.write(&oBlob);
        bufSrc.clear();
        bufSrc=QByteArray::fromRawData((char*)(oBlob.data()),oBlob.length());
        QPixmap dst;
        dst.loadFromData(bufSrc);
        bufSrc.clear();

        //qDebug() << "ImageMagick ms: " << tmr.elapsed();
        return dst;
    }
}

QColor ZGlobal::foregroundColor()
{
    qreal r,g,b;
    backgroundColor.getRgbF(&r,&g,&b);
    qreal br = r*0.2989+g*0.5870+b*0.1140;
    if (br>0.5)
        return QColor(Qt::black);
    else
        return QColor(Qt::white);
}

bool ZGlobal::sqlCheckBase()
{
    if (dbUser.isEmpty()) return false;

    MainWindow* w = qobject_cast<MainWindow *>(parent());
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) {
        QMessageBox::critical(w,tr("QManga"),tr("Unable to open MySQL connection. Check login info."));
        return false;
    }
    bool noTables = (!db.tables().contains("files",Qt::CaseInsensitive) ||
            !db.tables().contains("albums",Qt::CaseInsensitive));
    sqlCloseBase(db);
    if (noTables) {
        if (QMessageBox::question(w,tr("QManga"),tr("Database is empty. Recreate tables and structures?"),
                                  QMessageBox::Yes,QMessageBox::No)==QMessageBox::Yes)
            return sqlCreateTables();
        else
            return false;
    }
    return true;
}

bool ZGlobal::sqlCreateTables()
{
    MainWindow* w = qobject_cast<MainWindow *>(parent());

    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return false;

    QSqlQuery qr(db);
    if (!qr.exec("CREATE TABLE IF NOT EXISTS `albums` ("
                 "`id` int(11) NOT NULL AUTO_INCREMENT,"
                 "`name` varchar(2048) NOT NULL,"
                 "PRIMARY KEY (`id`)"
                 ") ENGINE=InnoDB DEFAULT CHARSET=utf8;")) {
        sqlCloseBase(db);
        QMessageBox::critical(w,tr("QManga"),tr("Unable to create table `albums`\n\n%1\n%2").
                              arg(qr.lastError().databaseText()).arg(qr.lastError().driverText()));
        return false;
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
        QMessageBox::critical(w,tr("QManga"),tr("Unable to create table `albums`\n\n%1\n%2").
                              arg(qr.lastError().databaseText()).arg(qr.lastError().driverText()));
        return false;
    }
    sqlCloseBase(db);
    return true;
}

QSqlDatabase ZGlobal::sqlOpenBase()
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

void ZGlobal::sqlCloseBase(QSqlDatabase &db)
{
    if (db.isOpen())
        db.close();
}

QStringList ZGlobal::sqlGetAlbums()
{
    QStringList res;
    res.clear();
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return res;

    QSqlQuery qr("SELECT name FROM `albums` ORDER BY name ASC",db);
    while (qr.next()) {
        res << qr.value(0).toString();
    }
    sqlCloseBase(db);
    return res;
}

void ZGlobal::sqlGetFiles(QString album, QString search, SQLMangaList* mList,
                          QMutex* listUpdating, ZMangaModel* model, ZOrdering order, bool reverseOrder)

{
    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    QSqlQuery qr(db);
    QString tqr;
    if (!album.isEmpty()) {
        tqr = QString("SELECT files.name, filename, cover, pagesCount, fileSize, fileMagic, fileDT, "
                      "addingDT, files.id, albums.name "
                      "FROM files LEFT JOIN albums ON files.album=albums.id "
                      "WHERE (album="
                      "  (SELECT id FROM albums WHERE (name = '%1'))"
                      ") ").arg(escapeParam(album));
    } else {
        tqr = QString("SELECT files.name, filename, cover, pagesCount, fileSize, fileMagic, fileDT, "
                "addingDT, files.id, albums.name "
                "FROM files LEFT JOIN albums ON files.album=albums.id "
                "WHERE (files.name LIKE '%%%1%%') ").arg(escapeParam(search));
    }
    if (order!=Unordered) {
        tqr+="ORDER BY ";
        if (order==FileDate) tqr+="fileDT";
        else if (order==AddingDate) tqr+="addingDT";
        else if (order==Album) tqr+="albums.name";
        else if (order==PagesCount) tqr+="pagesCount";
        else tqr+="files.name";
        if (reverseOrder) tqr+=" DESC";
        else tqr+=" ASC";
    }
    tqr+=";";
    qr.prepare(tqr);

    if (qr.exec()) {
        //int resCnt = qr.size();
        int idx = 0;
        while (qr.next()) {
            QImage p = QImage();
            QByteArray ba = qr.value(2).toByteArray();
            if (!ba.isEmpty())
                p.loadFromData(ba);

            listUpdating->lock();
            int posidx = mList->count();
            listUpdating->unlock();
            QMetaObject::invokeMethod(model,"rowsAboutToAppended",Qt::QueuedConnection,
                                      Q_ARG(int,posidx),
                                      Q_ARG(int,posidx+1));
            listUpdating->lock();
            *mList << SQLMangaEntry(qr.value(0).toString(),
                                    qr.value(1).toString(),
                                    album,
                                    p,
                                    qr.value(3).toInt(),
                                    qr.value(4).toInt(),
                                    qr.value(5).toString(),
                                    qr.value(6).toDateTime(),
                                    qr.value(7).toDateTime(),
                                    qr.value(8).toInt());
            listUpdating->unlock();
            QMetaObject::invokeMethod(model,"rowsAppended",Qt::QueuedConnection);
            idx++;
        }
    } else
        qDebug() << qr.lastError().databaseText() << qr.lastError().driverText();
    sqlCloseBase(db);
    return;
}

void ZGlobal::sqlAddFiles(QStringList aFiles, QString album)
{
    MainWindow* w = qobject_cast<MainWindow *>(parent());

    QStringList files = aFiles;
    files.removeDuplicates();

    QSqlDatabase db = sqlOpenBase();
    if (!db.isValid()) return;

    int ialbum = -1;

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
            QMessageBox::critical(w,tr("QManga"),tr("Unable to create album `%1`\n%2\n%3").
                                  arg(album).
                                  arg(qr.lastError().databaseText()).
                                  arg(qr.lastError().driverText()));
            sqlCloseBase(db);
            return;
        }
        ialbum = qr.lastInsertId().toInt();
    }

    QProgressDialog pd(tr("Adding files"),QString("Cancel"),0,100,w,Qt::Dialog);
    pd.setWindowModality(Qt::WindowModal);
    pd.setWindowTitle(tr("Adding files to index..."));
    pd.setValue(0);
    pd.show();

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
        QPixmap p = QPixmap();
        while (idx<za->getPageCount()) {
            QImage pi = za->loadPage(idx);
            if (!pi.isNull()) {
                p = QPixmap::fromImage(pi);
                p.detach();
                pi = QImage();
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
        pba.clear();
        p = QPixmap();
        za->closeFile();
        za->setParent(NULL);
        delete za;

        QApplication::processEvents();
        pd.setValue(100*i/files.count());
        QString s = fi.completeBaseName();
        if (s.length()>60) s = s.left(60)+"...";
        pd.setLabelText(s);

        if (pd.wasCanceled())
            break;
        QApplication::processEvents();
    }
    pd.hide();
    sqlCloseBase(db);
}

void ZGlobal::sqlDelFiles(QIntList dbids)
{
    MainWindow* w = qobject_cast<MainWindow *>(parent());
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
        QMessageBox::critical(w,tr("QManga"),
                              tr("Unable to delete manga\n%2\n%3").
                              arg(qr.lastError().databaseText()).
                              arg(qr.lastError().driverText()));
        sqlCloseBase(db);
        return;
    }
    sqlCloseBase(db);
    sqlDelEmptyAlbums();
}

void ZGlobal::sqlDelEmptyAlbums()
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
}

void ZGlobal::sqlRenameAlbum(QString oldName, QString newName)
{
    MainWindow* w = qobject_cast<MainWindow *>(parent());

    if (oldName==newName) return;
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
        QMessageBox::critical(w,tr("QManga"),tr("Unable to rename album `%1`\n%2\n%3").
                              arg(oldName).
                              arg(qr.lastError().databaseText()).
                              arg(qr.lastError().driverText()));
    }
    sqlCloseBase(db);
}

void ZGlobal::settingsDlg()
{
    MainWindow* w = qobject_cast<MainWindow *>(parent());
    SettingsDialog* dlg = new SettingsDialog(w);

    dlg->editMySqlLogin->setText(dbUser);
    dlg->editMySqlPassword->setText(dbPass);
    dlg->editMySqlBase->setText(dbBase);
    dlg->spinCacheWidth->setValue(cacheWidth);
    dlg->spinMagnify->setValue(magnifySize);
    dlg->updateBkColor(backgroundColor);
    switch (resizeFilter) {
    case Nearest: dlg->comboFilter->setCurrentIndex(0); break;
    case Bilinear: dlg->comboFilter->setCurrentIndex(1); break;
    case Lanczos: dlg->comboFilter->setCurrentIndex(2); break;
    case Gaussian: dlg->comboFilter->setCurrentIndex(3); break;
    case Lanczos2: dlg->comboFilter->setCurrentIndex(4); break;
    case Cubic: dlg->comboFilter->setCurrentIndex(5); break;
    case Sinc: dlg->comboFilter->setCurrentIndex(6); break;
    case Triangle: dlg->comboFilter->setCurrentIndex(7); break;
    case Mitchell: dlg->comboFilter->setCurrentIndex(8); break;
    }
    foreach (const QString &t, bookmarks.keys()) {
        QListWidgetItem* li = new QListWidgetItem(QString("%1 [ %2 ]").arg(t).arg(bookmarks.value(t)));
        li->setData(Qt::UserRole,t);
        li->setData(Qt::UserRole+1,bookmarks.value(t));
        dlg->listBookmarks->addItem(li);
    }

    if (dlg->exec()) {
        dbUser=dlg->editMySqlLogin->text();
        dbPass=dlg->editMySqlPassword->text();
        dbBase=dlg->editMySqlBase->text();
        cacheWidth=dlg->spinCacheWidth->value();
        magnifySize=dlg->spinMagnify->value();
        backgroundColor=dlg->getBkColor();
        switch (dlg->comboFilter->currentIndex()) {
        case 0: resizeFilter = Nearest; break;
        case 1: resizeFilter = Bilinear; break;
        case 2: resizeFilter = Lanczos; break;
        case 3: resizeFilter = Gaussian; break;
        case 4: resizeFilter = Lanczos2; break;
        case 5: resizeFilter = Cubic; break;
        case 6: resizeFilter = Sinc; break;
        case 7: resizeFilter = Triangle; break;
        case 8: resizeFilter = Mitchell; break;
        }
        bookmarks.clear();
        for (int i=0; i<dlg->listBookmarks->count(); i++)
            bookmarks[dlg->listBookmarks->item(i)->data(Qt::UserRole).toString()]=
                    dlg->listBookmarks->item(i)->data(Qt::UserRole+1).toString();
        if (w!=NULL) {
            w->updateBookmarks();
            w->updateViewer();
            w->srcWidget->setEnabled(!dbUser.isEmpty());
            if (w->srcWidget->isEnabled())
                w->srcWidget->updateAlbumsList();
        }
        sqlCheckBase();
    }
    dlg->setParent(NULL);
    delete dlg;
}

#ifdef QB_KDEDIALOGS
QString ZGlobal::getKDEFilters(const QString & qfilter)
{
    QStringList f = qfilter.split(";;",QString::SkipEmptyParts);
    QStringList r;
    r.clear();
    for (int i=0;i<f.count();i++) {
        QString s = f.at(i);
        if (s.indexOf('(')<0) continue;
        QString ftitle = s.left(s.indexOf('(')).trimmed();
        s.remove(0,s.indexOf('(')+1);
        if (s.indexOf(')')<0) continue;
        s = s.left(s.indexOf(')'));
        if (s.isEmpty()) continue;
        QStringList e = s.split(' ');
        for (int j=0;j<e.count();j++) {
            if (r.contains(e.at(j),Qt::CaseInsensitive)) continue;
            if (ftitle.isEmpty())
                r << QString("%1").arg(e.at(j));
            else
                r << QString("%1|%2").arg(e.at(j)).arg(ftitle);
        }
    }

    int idx;
    r.sort();
    idx=r.indexOf(QRegExp("^\\*\\.jpg.*",Qt::CaseInsensitive));
    if (idx>=0) { r.swap(0,idx); }
    idx=r.indexOf(QRegExp("^\\*\\.png.*",Qt::CaseInsensitive));
    if (idx>=0) { r.swap(1,0); r.swap(0,idx); }

    QString sf = r.join("\n");
    return sf;
}
#endif

QString ZGlobal::getOpenFileNameD ( QWidget * parent, const QString & caption, const QString & dir, const QString & filter, QString * selectedFilter, QFileDialog::Options options )
{
#ifdef QB_KDEDIALOGS
    ARGUNUSED(selectedFilter);
    ARGUNUSED(options);
    return KFileDialog::getOpenFileName(KUrl(dir),getKDEFilters(filter),parent,caption);
#else
    return QFileDialog::getOpenFileName(parent,caption,dir,filter,selectedFilter,options);
#endif
}

QStringList ZGlobal::getOpenFileNamesD ( QWidget * parent, const QString & caption, const QString & dir, const QString & filter, QString * selectedFilter, QFileDialog::Options options )
{
#ifdef QB_KDEDIALOGS
    ARGUNUSED(selectedFilter);
    ARGUNUSED(options);
    return KFileDialog::getOpenFileNames(KUrl(dir),getKDEFilters(filter),parent,caption);
#else
    return QFileDialog::getOpenFileNames(parent,caption,dir,filter,selectedFilter,options);
#endif
}

QString ZGlobal::getSaveFileNameD ( QWidget * parent, const QString & caption, const QString & dir, const QString & filter, QString * selectedFilter, QFileDialog::Options options )
{
#ifdef QB_KDEDIALOGS
    ARGUNUSED(selectedFilter);
    ARGUNUSED(options);
    return KFileDialog::getSaveFileName(KUrl(dir),getKDEFilters(filter),parent,caption);
#else
    return QFileDialog::getSaveFileName(parent,caption,dir,filter,selectedFilter,options);
#endif
}

QString	ZGlobal::getExistingDirectoryD ( QWidget * parent, const QString & caption, const QString & dir, QFileDialog::Options options )
{
#ifdef QB_KDEDIALOGS
    ARGUNUSED(options);
    return KFileDialog::getExistingDirectory(KUrl(dir),parent,caption);
#else
    return QFileDialog::getExistingDirectory(parent,caption,dir,options);
#endif
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

ZFileEntry::ZFileEntry()
{
    name = QString();
    idx = -1;
}

ZFileEntry::ZFileEntry(QString aName, int aIdx)
{
    name = aName;
    idx = aIdx;
}

ZFileEntry &ZFileEntry::operator =(const ZFileEntry &other)
{
    name = other.name;
    idx = other.idx;
    return *this;
}

bool ZFileEntry::operator ==(const ZFileEntry &ref) const
{
    return (name==ref.name);
}

bool ZFileEntry::operator !=(const ZFileEntry &ref) const
{
    return (name!=ref.name);
}

bool ZFileEntry::operator <(const ZFileEntry &ref) const
{
    QFileInfo fi1(name);
    QFileInfo fi2(ref.name);
    if (fi1.path()==fi2.path())
        return (compareWithNumerics(fi1.completeBaseName(),fi2.completeBaseName())<0);
    return (compareWithNumerics(fi1.path(),fi2.path())<0);
}

bool ZFileEntry::operator >(const ZFileEntry &ref) const
{
    QFileInfo fi1(name);
    QFileInfo fi2(ref.name);
    if (fi1.path()==fi2.path())
        return (compareWithNumerics(fi1.completeBaseName(),fi2.completeBaseName())>0);
    return (compareWithNumerics(fi1.path(),fi2.path())>0);
}
