#include <QDesktopWidget>
#include <QListView>
#include <QApplication>
#include <QThread>

#include "global.h"
#include "zglobal.h"
#include "mainwindow.h"
#include "settingsdialog.h"
#include "zzipreader.h"
#include "zmangamodel.h"
#include "zdb.h"
#include "zpdfreader.h"

ZGlobal* zg = NULL;

ZGlobal::ZGlobal(QObject *parent) :
    QObject(parent)
{
    cacheWidth = 6;
    detectedDelta = 120;
    bookmarks.clear();
    ctxSearchEngines.clear();
    defaultSearchEngine.clear();
    cachePixmaps = false;
    useFineRendering = true;
    ocrEditor = NULL;
    defaultOrdering = Z::Name;
    pdfRendering = Z::Autodetect;
    scrollDelta = 120;
    dpiX = 75.0;
    dpiY = 75.0;
    forceDPI = -1.0;

    filesystemWatcher = false;
    fsWatcher = new QFileSystemWatcher(this);
    newlyAddedFiles.clear();
    dirWatchList.clear();
    connect(fsWatcher,SIGNAL(directoryChanged(QString)),this,SLOT(directoryChanged(QString)));

    threadDB = new QThread();
    db = new ZDB();
    connect(this,&ZGlobal::dbSetCredentials,
            db,&ZDB::setCredentials,Qt::QueuedConnection);
    connect(this,&ZGlobal::dbCheckBase,db,&ZDB::sqlCheckBase,Qt::QueuedConnection);
    connect(this,&ZGlobal::dbCheckEmptyAlbums,db,&ZDB::sqlDelEmptyAlbums,Qt::QueuedConnection);
    connect(this,&ZGlobal::dbRescanIndexedDirs,db,&ZDB::sqlRescanIndexedDirs,Qt::QueuedConnection);
    connect(db,&ZDB::updateWatchDirList,
            this,&ZGlobal::updateWatchDirList,Qt::QueuedConnection);
    connect(db,&ZDB::baseCheckComplete,
            this,&ZGlobal::dbCheckComplete,Qt::QueuedConnection);
    connect(this,&ZGlobal::dbSetDynAlbums,db,&ZDB::setDynAlbums,Qt::QueuedConnection);
    connect(this,&ZGlobal::dbSetIgnoredFiles,
            db,&ZDB::sqlSetIgnoredFiles,Qt::QueuedConnection);
    db->moveToThread(threadDB);
    threadDB->start();

    initPdfReader();

    resetPreferredWidth();
}

ZGlobal::~ZGlobal()
{
    threadDB->quit();
    freePdfReader();
}

void ZGlobal::checkSQLProblems(QWidget *parent)
{
    QStrHash problems = db->getConfigProblems();
    if (problems.isEmpty()) return;

    MainWindow* mw = qobject_cast<MainWindow *>(parent);
    if (mw!=NULL) {
        mw->lblSearchStatus->setText(tr("%1 problems with MySQL").arg(problems.keys().count()));
        return;
    }
    QString text;
    foreach (const QString& msg, problems.keys()) {
        text += msg+"\n-------------------------------\n";
        text += problems.value(msg)+"\n\n";
    }
    QMessageBox mbox;
    mbox.setIcon(QMessageBox::Warning);
    mbox.setText(tr("Some problems found with MySQL configuration."));
    mbox.setInformativeText(tr("Please read warnings text in details below.\n"
                               "This list will be cleaned after QManga restart."));
    mbox.setDetailedText(text);
    mbox.setWindowTitle(tr("QManga"));
    mbox.exec();
}

void ZGlobal::loadSettings()
{
    MainWindow* w = qobject_cast<MainWindow *>(parent());

    QSettings settings("kernel1024", "qmanga");
    settings.beginGroup("MainWindow");
    cacheWidth = settings.value("cacheWidth",6).toInt();
    downscaleFilter = (Blitz::ScaleFilterType)settings.value("downscaleFilter",Blitz::LanczosFilter).toInt();
    upscaleFilter = (Blitz::ScaleFilterType)settings.value("upscaleFilter",Blitz::MitchellFilter).toInt();
    dbUser = settings.value("mysqlUser",QString()).toString();
    dbPass = settings.value("mysqlPassword",QString()).toString();
    dbBase = settings.value("mysqlBase",QString("qmanga")).toString();
    dbHost = settings.value("mysqlHost",QString("localhost")).toString();
    rarCmd = settings.value("rarCmd",QString()).toString();
    savedAuxOpenDir = settings.value("savedAuxOpenDir",QString()).toString();
    savedIndexOpenDir = settings.value("savedIndexOpenDir",QString()).toString();
    savedAuxSaveDir = settings.value("savedAuxSaveDir",QString()).toString();
    magnifySize = settings.value("magnifySize",150).toInt();
    resizeBlur = settings.value("resizingBlur",1.0).toFloat();
    scrollDelta = settings.value("scrollDelta",120).toInt();
    pdfRendering = (Z::PDFRendering)settings.value("pdfRendering",Z::Autodetect).toInt();
    forceDPI = settings.value("forceDPI",-1.0).toFloat();
    backgroundColor = QColor(settings.value("backgroundColor","#303030").toString());
    frameColor = QColor(settings.value("frameColor",QColor(Qt::lightGray).name()).toString());
    cachePixmaps = settings.value("cachePixmaps",false).toBool();
    useFineRendering = settings.value("fineRendering",true).toBool();
    filesystemWatcher = settings.value("filesystemWatcher",false).toBool();
    defaultOrdering = (Z::Ordering)settings.value("defaultOrdering",Z::Name).toInt();
    defaultSearchEngine = settings.value("defaultSearchEngine",QString()).toString();
    ctxSearchEngines = settings.value("ctxSearchEngines").value<ZStrMap>();


    if (!idxFont.fromString(settings.value("idxFont",QString()).toString()))
        idxFont = QApplication::font("QListView");
    if (!ocrFont.fromString(settings.value("ocrFont",QString()).toString()))
        ocrFont = QApplication::font("QPlainTextView");
    if (!backgroundColor.isValid())
        backgroundColor = QApplication::palette("QWidget").dark().color();
    if (!frameColor.isValid())
        frameColor = QColor(Qt::lightGray);

    bool showMaximized = false;
    if (w!=NULL)
        showMaximized = settings.value("maximized",false).toBool();

    bookmarks = settings.value("bookmarks").value<ZStrMap>();
    ZStrMap albums = settings.value("dynAlbums").value<ZStrMap>();
    emit dbSetDynAlbums(albums);

    if (w!=NULL) {
        QListView::ViewMode m = QListView::IconMode;
        if (settings.value("listMode",false).toBool())
            m = QListView::ListMode;
        w->searchTab->setListViewOptions(m, settings.value("iconSize",128).toInt());

        w->searchTab->loadSearchItems(settings);
    }

    settings.endGroup();

    emit dbSetCredentials(dbHost,dbBase,dbUser,dbPass);

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

    if (ocrEditor!=NULL)
        ocrEditor->setEditorFont(ocrFont);

    checkSQLProblems(w);
}

void ZGlobal::saveSettings()
{
    QSettings settings("kernel1024", "qmanga");
    settings.beginGroup("MainWindow");
    settings.remove("");
    settings.setValue("cacheWidth",cacheWidth);
    settings.setValue("downscaleFilter",(int)downscaleFilter);
    settings.setValue("upscaleFilter",(int)downscaleFilter);
    settings.setValue("mysqlUser",dbUser);
    settings.setValue("mysqlPassword",dbPass);
    settings.setValue("mysqlBase",dbBase);
    settings.setValue("mysqlHost",dbHost);
    settings.setValue("rarCmd",rarCmd);
    settings.setValue("savedAuxOpenDir",savedAuxOpenDir);
    settings.setValue("savedAuxSaveDir",savedAuxSaveDir);
    settings.setValue("savedIndexOpenDir",savedIndexOpenDir);
    settings.setValue("magnifySize",magnifySize);
    settings.setValue("resizingBlur",resizeBlur);
    settings.setValue("scrollDelta",scrollDelta);
    settings.setValue("pdfRendering",pdfRendering);
    settings.setValue("forceDPI",forceDPI);
    settings.setValue("backgroundColor",backgroundColor.name());
    settings.setValue("frameColor",frameColor.name());
    settings.setValue("cachePixmaps",cachePixmaps);
    settings.setValue("fineRendering",useFineRendering);
    settings.setValue("filesystemWatcher",filesystemWatcher);
    settings.setValue("idxFont",idxFont.toString());
    settings.setValue("ocrFont",ocrFont.toString());
    settings.setValue("defaultOrdering",(int)defaultOrdering);
    settings.setValue("defaultSearchEngine",defaultSearchEngine);
    settings.setValue("ctxSearchEngines",QVariant::fromValue(ctxSearchEngines));


    MainWindow* w = qobject_cast<MainWindow *>(parent());
    if (w!=NULL) {
        settings.setValue("maximized",w->isMaximized());

        settings.setValue("listMode",w->searchTab->getListViewMode()==QListView::ListMode);
        settings.setValue("iconSize",w->searchTab->getIconSize());

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

void ZGlobal::dbCheckComplete()
{
    MainWindow* w = qobject_cast<MainWindow *>(parent());
    checkSQLProblems(w);
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
    checkSQLProblems(NULL);

    MainWindow* w = qobject_cast<MainWindow *>(parent());
    SettingsDialog* dlg = new SettingsDialog(w);

    dlg->editMySqlLogin->setText(dbUser);
    dlg->editMySqlPassword->setText(dbPass);
    dlg->editMySqlBase->setText(dbBase);
    dlg->editMySqlHost->setText(dbHost);
    dlg->editRar->setText(rarCmd);
    dlg->spinCacheWidth->setValue(cacheWidth);
    dlg->spinMagnify->setValue(magnifySize);
    dlg->spinBlur->setValue(resizeBlur);
    dlg->spinScrollDelta->setValue(scrollDelta);
    dlg->labelDetectedDelta->setText(tr("Detected delta per one scroll event: %1 deg").arg(detectedDelta));
    if (cachePixmaps)
        dlg->radioCachePixmaps->setChecked(true);
    else
        dlg->radioCacheData->setChecked(true);
    dlg->checkFineRendering->setChecked(useFineRendering);
    dlg->updateBkColor(backgroundColor);
    dlg->updateIdxFont(idxFont);
    dlg->updateOCRFont(ocrFont);
    dlg->updateFrameColor(frameColor);
    dlg->checkFSWatcher->setChecked(filesystemWatcher);
    dlg->comboUpscaleFilter->setCurrentIndex((int)upscaleFilter);
    dlg->comboDownscaleFilter->setCurrentIndex((int)downscaleFilter);
    dlg->comboPDFRendererMode->setCurrentIndex((int)pdfRendering);
    dlg->spinForceDPI->setEnabled(forceDPI>0.0);
    dlg->checkForceDPI->setChecked(forceDPI>0.0);
    if (forceDPI<=0.0)
        dlg->spinForceDPI->setValue(dpiX);
    else
        dlg->spinForceDPI->setValue(forceDPI);

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
    dlg->setIgnoredFiles(db->sqlGetIgnoredFiles());
    dlg->setSearchEngines(ctxSearchEngines);

    if (dlg->exec()) {
        dbUser=dlg->editMySqlLogin->text();
        dbPass=dlg->editMySqlPassword->text();
        dbBase=dlg->editMySqlBase->text();
        dbHost=dlg->editMySqlHost->text();
        rarCmd=dlg->editRar->text();
        cacheWidth=dlg->spinCacheWidth->value();
        magnifySize=dlg->spinMagnify->value();
        resizeBlur=dlg->spinBlur->value();
        scrollDelta=dlg->spinScrollDelta->value();
        backgroundColor=dlg->getBkColor();
        frameColor=dlg->getFrameColor();
        idxFont=dlg->getIdxFont();
        ocrFont=dlg->getOCRFont();
        cachePixmaps=dlg->radioCachePixmaps->isChecked();
        useFineRendering=dlg->checkFineRendering->isChecked();
        filesystemWatcher=dlg->checkFSWatcher->isChecked();
        upscaleFilter=(Blitz::ScaleFilterType)dlg->comboUpscaleFilter->currentIndex();
        downscaleFilter=(Blitz::ScaleFilterType)dlg->comboDownscaleFilter->currentIndex();
        pdfRendering = (Z::PDFRendering)dlg->comboPDFRendererMode->currentIndex();
        ctxSearchEngines=dlg->getSearchEngines();
        if (dlg->checkForceDPI->isChecked())
            forceDPI = dlg->spinForceDPI->value();
        else
            forceDPI = -1.0;
        bookmarks.clear();
        for (int i=0; i<dlg->listBookmarks->count(); i++)
            bookmarks[dlg->listBookmarks->item(i)->data(Qt::UserRole).toString()]=
                    dlg->listBookmarks->item(i)->data(Qt::UserRole+1).toString();
        emit dbSetCredentials(dbHost,dbBase,dbUser,dbPass);
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
        emit dbSetIgnoredFiles(dlg->getIgnoredFiles());
        if (filesystemWatcher)
            emit dbRescanIndexedDirs();
        if (ocrEditor!=NULL)
            ocrEditor->setEditorFont(ocrFont);
    }
    dlg->setParent(NULL);
    delete dlg;
}

QUrl ZGlobal::createSearchUrl(const QString& text, const QString& engine)
{
    if (ctxSearchEngines.isEmpty())
        return QUrl();

    QString url = ctxSearchEngines.values().first();
    if (engine.isEmpty() && !defaultSearchEngine.isEmpty())
        url = ctxSearchEngines.value(defaultSearchEngine);
    if (!engine.isEmpty() && ctxSearchEngines.contains(engine))
        url = ctxSearchEngines.value(engine);

    url.replace("%s",text);
    url.replace("%ps",QUrl::toPercentEncoding(text));

    return QUrl::fromUserInput(url);
}


ZFileEntry::ZFileEntry()
{
    name = QString();
    idx = -1;
}

ZFileEntry::ZFileEntry(const ZFileEntry &other)
{
    name = other.name;
    idx = other.idx;
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
