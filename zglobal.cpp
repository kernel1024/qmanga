#include <QDesktopWidget>
#include <QApplication>

#include "global.h"
#include "zglobal.h"
#include "mainwindow.h"
#include "settingsdialog.h"
#include "zzipreader.h"
#include "zmangamodel.h"
#include "zdb.h"

ZGlobal* zg = NULL;

ZGlobal::ZGlobal(QObject *parent) :
    QObject(parent)
{
    cacheWidth = 6;
    detectedDelta = 120;
    bookmarks.clear();
    cachePixmaps = false;
    useFineRendering = true;
    ocrEditor = NULL;
    defaultOrdering = Z::FileName;
    scrollDelta = 120;

    filesystemWatcher = false;
    fsWatcher = new QFileSystemWatcher(this);
    newlyAddedFiles.clear();
    dirWatchList.clear();
    connect(fsWatcher,SIGNAL(directoryChanged(QString)),this,SLOT(directoryChanged(QString)));

    threadDB = new QThread();
    db = new ZDB();
    connect(this,SIGNAL(dbSetCredentials(QString,QString,QString)),
            db,SLOT(setCredentials(QString,QString,QString)),Qt::QueuedConnection);
    connect(this,SIGNAL(dbCheckBase()),db,SLOT(sqlCheckBase()),Qt::QueuedConnection);
    connect(this,SIGNAL(dbCheckEmptyAlbums()),db,SLOT(sqlDelEmptyAlbums()),Qt::QueuedConnection);
    connect(this,SIGNAL(dbRescanIndexedDirs()),db,SLOT(sqlRescanIndexedDirs()),Qt::QueuedConnection);
    connect(db,SIGNAL(updateWatchDirList(QStringList)),
            this,SLOT(updateWatchDirList(QStringList)),Qt::QueuedConnection);
    connect(this,SIGNAL(dbSetDynAlbums(ZStrMap)),db,SLOT(setDynAlbums(ZStrMap)),Qt::QueuedConnection);
    db->moveToThread(threadDB);
    threadDB->start();

    resetPreferredWidth();
}

ZGlobal::~ZGlobal()
{
    threadDB->quit();
}

void ZGlobal::loadSettings()
{
    MainWindow* w = qobject_cast<MainWindow *>(parent());

    QSettings settings("kernel1024", "qmanga");
    settings.beginGroup("MainWindow");
    cacheWidth = settings.value("cacheWidth",6).toInt();
    resizeFilter = (Z::ResizeFilter)settings.value("resizeFilter",0).toInt();
    dbUser = settings.value("mysqlUser",QString()).toString();
    dbPass = settings.value("mysqlPassword",QString()).toString();
    dbBase = settings.value("mysqlBase",QString("qmanga")).toString();
    savedAuxOpenDir = settings.value("savedAuxOpenDir",QString()).toString();
    savedIndexOpenDir = settings.value("savedIndexOpenDir",QString()).toString();
    magnifySize = settings.value("magnifySize",150).toInt();
    scrollDelta = settings.value("scrollDelta",120).toInt();
    backgroundColor = QColor(settings.value("backgroundColor","#303030").toString());
    frameColor = QColor(settings.value("frameColor",QColor(Qt::lightGray).name()).toString());
    cachePixmaps = settings.value("cachePixmaps",false).toBool();
    useFineRendering = settings.value("fineRendering",true).toBool();
    filesystemWatcher = settings.value("filesystemWatcher",false).toBool();
    defaultOrdering = (Z::Ordering)settings.value("defaultOrdering",Z::FileName).toInt();
    if (!idxFont.fromString(settings.value("idxFont",QString()).toString()))
        idxFont = QApplication::font("QListView");
    if (!backgroundColor.isValid())
        backgroundColor = QApplication::palette("QWidget").dark().color();
    if (!frameColor.isValid())
        frameColor = QColor(Qt::lightGray);

    bool showMaximized = false;
    if (w!=NULL)
        showMaximized = settings.value("maximized",false).toBool();

    int sz = settings.beginReadArray("bookmarks");
    if (sz>0) {
        for (int i=0; i<sz; i++) {
            settings.setArrayIndex(i);
            QString t = settings.value("title").toString();
            if (!t.isEmpty())
                bookmarks[t]=settings.value("url").toString();
        }
        settings.endArray();
    } else {
        settings.endArray();
        bookmarks = settings.value("bookmarks").value<ZStrMap>();
    }

    ZStrMap albums;
    albums.clear();
    sz = settings.beginReadArray("dynAlbums");
    if (sz>0) {
        for (int i=0; i<sz; i++) {
            settings.setArrayIndex(i);
            QString t = settings.value("title").toString();
            if (!t.isEmpty())
                albums[t]=settings.value("query").toString();
        }
        settings.endArray();
    } else {
        settings.endArray();
        albums = settings.value("dynAlbums").value<ZStrMap>();
    }
    emit dbSetDynAlbums(albums);

    if (w!=NULL) {
        if (settings.value("listMode",false).toBool())
            w->searchTab->srclModeList->setChecked(true);
        else
            w->searchTab->srclModeIcon->setChecked(true);
        w->searchTab->srclIconSize->setValue(settings.value("iconSize",128).toInt());

        w->searchTab->loadSearchItems(settings);
    }

    settings.endGroup();

    emit dbSetCredentials(dbBase,dbUser,dbPass);

    if (w!=NULL) {
        if (showMaximized)
            w->showMaximized();

        w->updateBookmarks();
        w->updateViewer();
        w->searchTab->setEnabled(!dbUser.isEmpty());
        if (w->searchTab->isEnabled()) {
            w->searchTab->updateAlbumsList();
            w->searchTab->applyOrder(defaultOrdering,false,false);
        }
    }
    emit dbCheckBase();
    if (filesystemWatcher)
        emit dbRescanIndexedDirs();
}

void ZGlobal::saveSettings()
{
    QSettings settings("kernel1024", "qmanga");
    settings.beginGroup("MainWindow");
    settings.remove("");
    settings.setValue("cacheWidth",cacheWidth);
    settings.setValue("resizeFilter",(int)resizeFilter);
    settings.setValue("mysqlUser",dbUser);
    settings.setValue("mysqlPassword",dbPass);
    settings.setValue("mysqlBase",dbBase);
    settings.setValue("savedAuxOpenDir",savedAuxOpenDir);
    settings.setValue("savedIndexOpenDir",savedIndexOpenDir);
    settings.setValue("magnifySize",magnifySize);
    settings.setValue("scrollDelta",scrollDelta);
    settings.setValue("backgroundColor",backgroundColor.name());
    settings.setValue("frameColor",frameColor.name());
    settings.setValue("cachePixmaps",cachePixmaps);
    settings.setValue("fineRendering",useFineRendering);
    settings.setValue("filesystemWatcher",filesystemWatcher);
    settings.setValue("idxFont",idxFont.toString());
    settings.setValue("defaultOrdering",(int)defaultOrdering);

    MainWindow* w = qobject_cast<MainWindow *>(parent());
    if (w!=NULL) {
        settings.setValue("maximized",w->isMaximized());

        settings.setValue("listMode",w->searchTab->srclModeList->isChecked());
        settings.setValue("iconSize",w->searchTab->srclIconSize->value());

        w->searchTab->saveSearchItems(settings);
    }

    settings.setValue("bookmarks",QVariant::fromValue(bookmarks));
    settings.setValue("dynAlbums",QVariant::fromValue(db->getDynAlbums()));

    settings.endGroup();

    emit dbCheckEmptyAlbums();
}

void ZGlobal::updateWatchDirList(const QStringList &watchDirs)
{
    dirWatchList.clear();
    for (int i=0;i<watchDirs.count();i++) {
        QDir d(watchDirs.at(i));
        if (!d.isReadable()) continue;
        dirWatchList[d.absolutePath()] =
                d.entryList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
    }
    fsWatcher->addPaths(watchDirs);
}

void ZGlobal::directoryChanged(const QString &dir)
{
    QDir d(dir);
    if (!d.isReadable()) return;
    QStringList nlist = d.entryList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
    QStringList olist = dirWatchList[d.absolutePath()];
    for (int i=0;i<nlist.count();i++) {
        QString fname = d.absoluteFilePath(nlist.at(i));
        if (!olist.contains(nlist.at(i)) && !newlyAddedFiles.contains(fname))
            newlyAddedFiles << fname;
    }
    fsCheckFilesAvailability();
}

void ZGlobal::resetPreferredWidth()
{
    int screen = 0;
    QDesktopWidget *desktop = QApplication::desktop();
    if (desktop->isVirtualDesktop()) {
        screen = desktop->screenNumber(QCursor::pos());
    }

    preferredWidth = 2*desktop->screen(screen)->width()/3;
    if (preferredWidth<1000)
        preferredWidth = 1000;
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

void ZGlobal::fsCheckFilesAvailability()
{
    int i = 0;
    while (i<newlyAddedFiles.count() && !newlyAddedFiles.isEmpty()) {
        QFileInfo fi(newlyAddedFiles.at(i));
        if (!(fi.isReadable() && fi.isFile()))
            newlyAddedFiles.removeAt(i);
        else
            i++;
    }
    emit fsFilesAdded();
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
    dlg->spinScrollDelta->setValue(scrollDelta);
    dlg->labelDetectedDelta->setText(tr("Detected delta per one scroll event: %1 deg").arg(detectedDelta));
    if (cachePixmaps)
        dlg->radioCachePixmaps->setChecked(true);
    else
        dlg->radioCacheData->setChecked(true);
    dlg->checkFineRendering->setChecked(useFineRendering);
    dlg->updateBkColor(backgroundColor);
    dlg->updateIdxFont(idxFont);
    dlg->updateFrameColor(frameColor);
    dlg->checkFSWatcher->setChecked(filesystemWatcher);
    switch (resizeFilter) {
        case Z::Nearest: dlg->comboFilter->setCurrentIndex(0); break;
        case Z::Bilinear: dlg->comboFilter->setCurrentIndex(1); break;
        case Z::Lanczos: dlg->comboFilter->setCurrentIndex(2); break;
        case Z::Gaussian: dlg->comboFilter->setCurrentIndex(3); break;
        case Z::Lanczos2: dlg->comboFilter->setCurrentIndex(4); break;
        case Z::Cubic: dlg->comboFilter->setCurrentIndex(5); break;
        case Z::Sinc: dlg->comboFilter->setCurrentIndex(6); break;
        case Z::Triangle: dlg->comboFilter->setCurrentIndex(7); break;
        case Z::Mitchell: dlg->comboFilter->setCurrentIndex(8); break;
    }
    foreach (const QString &t, bookmarks.keys()) {
        QString st = bookmarks.value(t);
        if (st.split('\n').count()>0)
            st = st.split('\n').at(0);
        QListWidgetItem* li = new QListWidgetItem(QString("%1 [ %2 ]").arg(t).arg(st));
        li->setData(Qt::UserRole,t);
        li->setData(Qt::UserRole+1,bookmarks.value(t));
        dlg->listBookmarks->addItem(li);
    }
    ZStrMap albums = db->getDynAlbums();
    foreach (const QString &title, albums.keys()) {
        QString query = albums.value(title);
        QListWidgetItem* li = new QListWidgetItem(QString("%1 [ %2 ]").arg(title).arg(query));
        li->setData(Qt::UserRole,title);
        li->setData(Qt::UserRole+1,query);
        dlg->listDynAlbums->addItem(li);
    }

    if (dlg->exec()) {
        dbUser=dlg->editMySqlLogin->text();
        dbPass=dlg->editMySqlPassword->text();
        dbBase=dlg->editMySqlBase->text();
        cacheWidth=dlg->spinCacheWidth->value();
        magnifySize=dlg->spinMagnify->value();
        scrollDelta=dlg->spinScrollDelta->value();
        backgroundColor=dlg->getBkColor();
        frameColor=dlg->getFrameColor();
        idxFont=dlg->getIdxFont();
        cachePixmaps=dlg->radioCachePixmaps->isChecked();
        useFineRendering=dlg->checkFineRendering->isChecked();
        filesystemWatcher=dlg->checkFSWatcher->isChecked();
        switch (dlg->comboFilter->currentIndex()) {
            case 0: resizeFilter = Z::Nearest; break;
            case 1: resizeFilter = Z::Bilinear; break;
            case 2: resizeFilter = Z::Lanczos; break;
            case 3: resizeFilter = Z::Gaussian; break;
            case 4: resizeFilter = Z::Lanczos2; break;
            case 5: resizeFilter = Z::Cubic; break;
            case 6: resizeFilter = Z::Sinc; break;
            case 7: resizeFilter = Z::Triangle; break;
            case 8: resizeFilter = Z::Mitchell; break;
        }
        bookmarks.clear();
        for (int i=0; i<dlg->listBookmarks->count(); i++)
            bookmarks[dlg->listBookmarks->item(i)->data(Qt::UserRole).toString()]=
                    dlg->listBookmarks->item(i)->data(Qt::UserRole+1).toString();
        emit dbSetCredentials(dbBase,dbUser,dbPass);
        albums.clear();
        for (int i=0; i<dlg->listDynAlbums->count(); i++)
            albums[dlg->listDynAlbums->item(i)->data(Qt::UserRole).toString()]=
                    dlg->listDynAlbums->item(i)->data(Qt::UserRole+1).toString();
        emit dbSetDynAlbums(albums);
        if (w!=NULL) {
            w->updateBookmarks();
            w->updateViewer();
            w->searchTab->setEnabled(!dbUser.isEmpty());
            if (w->searchTab->isEnabled())
                w->searchTab->updateAlbumsList();
        }
        emit dbCheckBase();
        if (filesystemWatcher)
            emit dbRescanIndexedDirs();
    }
    dlg->setParent(NULL);
    delete dlg;
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
