#include <QWindow>
#include <QScreen>
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
#include "zdjvureader.h"

ZGlobal* zg = nullptr;

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
    ocrEditor = nullptr;
    defaultOrdering = Z::Name;
    pdfRendering = Z::Autodetect;
    dbEngine = Z::SQLite;
    scrollDelta = 120;
    scrollFactor = 3;
    searchScrollFactor = 0.1;
    dpiX = 75.0;
    dpiY = 75.0;
    forceDPI = -1.0;
    downscaleFilter = Blitz::ScaleFilterType::UndefinedFilter;
    upscaleFilter = Blitz::ScaleFilterType::UndefinedFilter;
    magnifySize = 100;
    resizeBlur = 1.0;

    initLanguagesList();
    tranSourceLang = QStringLiteral("ja");
    tranDestLang = QStringLiteral("en");

    filesystemWatcher = false;
    fsWatcher = new QFileSystemWatcher(this);
    newlyAddedFiles.clear();
    dirWatchList.clear();
    connect(fsWatcher,&QFileSystemWatcher::directoryChanged,this,&ZGlobal::directoryChanged);

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
    connect(threadDB,&QThread::finished,db,&QObject::deleteLater,Qt::QueuedConnection);
    connect(threadDB,&QThread::finished,threadDB,&QObject::deleteLater,Qt::QueuedConnection);

    threadDB->start();

    initPdfReader();
    ZDjVuController::instance()->initDjVuReader();
}

ZGlobal::~ZGlobal()
{
    threadDB->quit();
    QApplication::processEvents();
    freePdfReader();
    ZDjVuController::instance()->freeDjVuReader();
}

void ZGlobal::checkSQLProblems(QWidget *parent)
{
    QStrHash problems = db->getConfigProblems();
    if (problems.isEmpty()) return;

    auto mw = qobject_cast<MainWindow *>(parent);
    if (mw!=nullptr) {
        mw->lblSearchStatus->setText(tr("%1 problems with MySQL").arg(problems.count()));
        return;
    }
    QString text;
    for (auto it=problems.constKeyValueBegin(), end=problems.constKeyValueEnd(); it!=end; ++it) {
        text.append(QStringLiteral("%1\n-------------------------------\n%2\n\n")
                    .arg((*it).first,(*it).second));
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
    auto w = qobject_cast<MainWindow *>(parent());

    QSettings settings(QStringLiteral("kernel1024"), QStringLiteral("qmanga"));
    settings.beginGroup(QStringLiteral("MainWindow"));
    cacheWidth = settings.value(QStringLiteral("cacheWidth"),6).toInt();
    downscaleFilter = static_cast<Blitz::ScaleFilterType>(
                          settings.value(QStringLiteral("downscaleFilter"),Blitz::LanczosFilter).toInt());
    upscaleFilter = static_cast<Blitz::ScaleFilterType>(
                        settings.value(QStringLiteral("upscaleFilter"),Blitz::MitchellFilter).toInt());
    dbUser = settings.value(QStringLiteral("mysqlUser"),QString()).toString();
    dbPass = settings.value(QStringLiteral("mysqlPassword"),QString()).toString();
    dbBase = settings.value(QStringLiteral("mysqlBase"),QStringLiteral("qmanga")).toString();
    dbHost = settings.value(QStringLiteral("mysqlHost"),QStringLiteral("localhost")).toString();
    rarCmd = settings.value(QStringLiteral("rarCmd"),QString()).toString();
    savedAuxOpenDir = settings.value(QStringLiteral("savedAuxOpenDir"),QString()).toString();
    savedIndexOpenDir = settings.value(QStringLiteral("savedIndexOpenDir"),QString()).toString();
    savedAuxSaveDir = settings.value(QStringLiteral("savedAuxSaveDir"),QString()).toString();
    magnifySize = settings.value(QStringLiteral("magnifySize"),150).toInt();
    resizeBlur = settings.value(QStringLiteral("resizingBlur"),1.0).toDouble();
    scrollDelta = settings.value(QStringLiteral("scrollDelta"),120).toInt();
    scrollFactor = settings.value(QStringLiteral("scrollFactor"),5).toInt();
    searchScrollFactor = settings.value(QStringLiteral("searchScrollFactor"),0.1).toDouble();
    pdfRendering = static_cast<Z::PDFRendering>(
                       settings.value(QStringLiteral("pdfRendering"),
                                      Z::Autodetect).toInt());
    dbEngine = static_cast<Z::DBMS>(settings.value(QStringLiteral("dbEngine"),Z::SQLite).toInt());
    forceDPI = settings.value(QStringLiteral("forceDPI"),-1.0).toDouble();
    backgroundColor = QColor(settings.value(QStringLiteral("backgroundColor"),
                                            QStringLiteral("#303030")).toString());
    frameColor = QColor(settings.value(QStringLiteral("frameColor"),
                                       QColor(Qt::lightGray).name()).toString());
    cachePixmaps = settings.value(QStringLiteral("cachePixmaps"),false).toBool();
    useFineRendering = settings.value(QStringLiteral("fineRendering"),true).toBool();
    filesystemWatcher = settings.value(QStringLiteral("filesystemWatcher"),false).toBool();
    defaultOrdering = static_cast<Z::Ordering>(settings.value(QStringLiteral("defaultOrdering"),
                                                              Z::Name).toInt());
    defaultSearchEngine = settings.value(QStringLiteral("defaultSearchEngine"),QString()).toString();
    ctxSearchEngines = settings.value(QStringLiteral("ctxSearchEngines")).value<ZStrMap>();


    if (!idxFont.fromString(settings.value(QStringLiteral("idxFont"),QString()).toString()))
        idxFont = QApplication::font("QListView");
    if (!ocrFont.fromString(settings.value(QStringLiteral("ocrFont"),QString()).toString()))
        ocrFont = QApplication::font("QPlainTextView");
    if (!backgroundColor.isValid())
        backgroundColor = QApplication::palette("QWidget").dark().color();
    if (!frameColor.isValid())
        frameColor = QColor(Qt::lightGray);

    bool showMaximized = false;
    if (w!=nullptr)
        showMaximized = settings.value(QStringLiteral("maximized"),false).toBool();

    bookmarks = settings.value(QStringLiteral("bookmarks")).value<ZStrMap>();
    ZStrMap albums = settings.value(QStringLiteral("dynAlbums")).value<ZStrMap>();
    emit dbSetDynAlbums(albums);

    if (w!=nullptr) {
        QListView::ViewMode m = QListView::IconMode;
        if (settings.value(QStringLiteral("listMode"),false).toBool())
            m = QListView::ListMode;
        w->searchTab->setListViewOptions(m, settings.value(QStringLiteral("iconSize"),128).toInt());

        w->searchTab->loadSearchItems(settings);
    }

    settings.endGroup();

    emit dbSetCredentials(dbHost,dbBase,dbUser,dbPass);

    if (w!=nullptr) {
        if (showMaximized)
            w->showMaximized();

        w->updateBookmarks();
        w->updateViewer();
        w->searchTab->setEnabled(dbEngine!=Z::UndefinedDB);
        if (w->searchTab->isEnabled()) {
            w->searchTab->updateAlbumsList();
            w->searchTab->applySortOrder(defaultOrdering);
        }
    }
    emit dbCheckBase();
    if (filesystemWatcher)
        emit dbRescanIndexedDirs();

    if (ocrEditor!=nullptr)
        ocrEditor->setEditorFont(ocrFont);

    checkSQLProblems(w);
}

void ZGlobal::saveSettings()
{
    QSettings settings(QStringLiteral("kernel1024"), QStringLiteral("qmanga"));
    settings.beginGroup(QStringLiteral("MainWindow"));
    settings.remove(QString());
    settings.setValue(QStringLiteral("cacheWidth"),cacheWidth);
    settings.setValue(QStringLiteral("downscaleFilter"),static_cast<int>(downscaleFilter));
    settings.setValue(QStringLiteral("upscaleFilter"),static_cast<int>(upscaleFilter));
    settings.setValue(QStringLiteral("mysqlUser"),dbUser);
    settings.setValue(QStringLiteral("mysqlPassword"),dbPass);
    settings.setValue(QStringLiteral("mysqlBase"),dbBase);
    settings.setValue(QStringLiteral("mysqlHost"),dbHost);
    settings.setValue(QStringLiteral("rarCmd"),rarCmd);
    settings.setValue(QStringLiteral("savedAuxOpenDir"),savedAuxOpenDir);
    settings.setValue(QStringLiteral("savedAuxSaveDir"),savedAuxSaveDir);
    settings.setValue(QStringLiteral("savedIndexOpenDir"),savedIndexOpenDir);
    settings.setValue(QStringLiteral("magnifySize"),magnifySize);
    settings.setValue(QStringLiteral("resizingBlur"),resizeBlur);
    settings.setValue(QStringLiteral("scrollDelta"),scrollDelta);
    settings.setValue(QStringLiteral("scrollFactor"),scrollFactor);
    settings.setValue(QStringLiteral("searchScrollFactor"),searchScrollFactor);
    settings.setValue(QStringLiteral("pdfRendering"),pdfRendering);
    settings.setValue(QStringLiteral("dbEngine"),dbEngine);
    settings.setValue(QStringLiteral("forceDPI"),forceDPI);
    settings.setValue(QStringLiteral("backgroundColor"),backgroundColor.name());
    settings.setValue(QStringLiteral("frameColor"),frameColor.name());
    settings.setValue(QStringLiteral("cachePixmaps"),cachePixmaps);
    settings.setValue(QStringLiteral("fineRendering"),useFineRendering);
    settings.setValue(QStringLiteral("filesystemWatcher"),filesystemWatcher);
    settings.setValue(QStringLiteral("idxFont"),idxFont.toString());
    settings.setValue(QStringLiteral("ocrFont"),ocrFont.toString());
    settings.setValue(QStringLiteral("defaultOrdering"),static_cast<int>(defaultOrdering));
    settings.setValue(QStringLiteral("defaultSearchEngine"),defaultSearchEngine);
    settings.setValue(QStringLiteral("ctxSearchEngines"),QVariant::fromValue(ctxSearchEngines));
    settings.setValue(QStringLiteral("tranSourceLanguage"),tranSourceLang);
    settings.setValue(QStringLiteral("tranDestLanguage"),tranDestLang);

    auto w = qobject_cast<MainWindow *>(parent());
    if (w!=nullptr) {
        settings.setValue(QStringLiteral("maximized"),w->isMaximized());

        settings.setValue(QStringLiteral("listMode"),w->searchTab->getListViewMode()==QListView::ListMode);
        settings.setValue(QStringLiteral("iconSize"),w->searchTab->getIconSize());

        w->searchTab->saveSearchItems(settings);
    }

    settings.setValue(QStringLiteral("bookmarks"),QVariant::fromValue(bookmarks));
    settings.setValue(QStringLiteral("dynAlbums"),QVariant::fromValue(db->getDynAlbums()));

    settings.endGroup();

    emit dbCheckEmptyAlbums();
}

void ZGlobal::updateWatchDirList(const QStringList &watchDirs)
{
    dirWatchList.clear();
    for (const auto &i : watchDirs) {
        QDir d(i);
        if (!d.isReadable()) continue;
        dirWatchList[d.absolutePath()] =
                d.entryList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
    }
    fsWatcher->addPaths(watchDirs);
}

void ZGlobal::directoryChanged(const QString &dir)
{
    QStringList ignoredFiles;
    if (db!=nullptr)
        ignoredFiles = db->sqlGetIgnoredFiles();

    QDir d(dir);
    if (!d.isReadable()) return;
    const QStringList nlist = d.entryList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
    const QStringList olist = dirWatchList.value(d.absolutePath());
    for (const auto &i : nlist) {
        QString fname = d.absoluteFilePath(i);
        if (!olist.contains(i) && !newlyAddedFiles.contains(fname) &&
                !ignoredFiles.contains(fname))
            newlyAddedFiles << fname;
    }
    fsCheckFilesAvailability();
}

void ZGlobal::dbCheckComplete()
{
    auto w = qobject_cast<MainWindow *>(parent());
    checkSQLProblems(w);
}

QColor ZGlobal::foregroundColor()
{
    qreal r,g,b;
    backgroundColor.getRgbF(&r,&g,&b);
    qreal br = r*0.2989+g*0.5870+b*0.1140;
    if (br>0.5)
        return QColor(Qt::black);

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
    checkSQLProblems(nullptr);

    auto w = qobject_cast<MainWindow *>(parent());
    auto dlg = new SettingsDialog(w);

    dlg->editMySqlLogin->setText(dbUser);
    dlg->editMySqlPassword->setText(dbPass);
    dlg->editMySqlBase->setText(dbBase);
    dlg->editMySqlHost->setText(dbHost);
    dlg->editRar->setText(rarCmd);
    dlg->spinCacheWidth->setValue(cacheWidth);
    dlg->spinMagnify->setValue(magnifySize);
    dlg->spinBlur->setValue(resizeBlur);
    dlg->spinScrollDelta->setValue(scrollDelta);
    dlg->spinScrollFactor->setValue(scrollFactor);
    dlg->spinSearchScrollFactor->setValue(searchScrollFactor);
    dlg->labelDetectedDelta->setText(tr("Detected delta per one scroll event: %1 deg").arg(detectedDelta));
    if (cachePixmaps)
        dlg->radioCachePixmaps->setChecked(true);
    else
        dlg->radioCacheData->setChecked(true);
    if (dbEngine==Z::MySQL)
        dlg->radioMySQL->setChecked(true);
    else if (dbEngine==Z::SQLite)
        dlg->radioSQLite->setChecked(true);
    else
        dlg->radioSQLite->setChecked(true);
    dlg->checkFineRendering->setChecked(useFineRendering);
    dlg->updateBkColor(backgroundColor);
    dlg->updateIdxFont(idxFont);
    dlg->updateOCRFont(ocrFont);
    dlg->updateFrameColor(frameColor);
    dlg->checkFSWatcher->setChecked(filesystemWatcher);
    dlg->comboUpscaleFilter->setCurrentIndex(static_cast<int>(upscaleFilter));
    dlg->comboDownscaleFilter->setCurrentIndex(static_cast<int>(downscaleFilter));
    dlg->comboPDFRendererMode->setCurrentIndex(static_cast<int>(pdfRendering));
    dlg->spinForceDPI->setEnabled(forceDPI>0.0);
    dlg->checkForceDPI->setChecked(forceDPI>0.0);
    if (forceDPI<=0.0)
        dlg->spinForceDPI->setValue(dpiX);
    else
        dlg->spinForceDPI->setValue(forceDPI);

    for (auto bIt=bookmarks.constKeyValueBegin(), end=bookmarks.constKeyValueEnd(); bIt!=end; ++bIt) {
        QString st = (*bIt).second;
        if (st.split('\n').count()>0)
            st = st.split('\n').at(0);
        QListWidgetItem* li = new QListWidgetItem(QStringLiteral("%1 [ %2 ]").arg((*bIt).first,
                                                                           (*bIt).second));
        li->setData(Qt::UserRole,(*bIt).first);
        li->setData(Qt::UserRole+1,(*bIt).second);
        dlg->listBookmarks->addItem(li);
    }
    ZStrMap albums = db->getDynAlbums();
    for (auto tIt = albums.constKeyValueBegin(), end = albums.constKeyValueEnd(); tIt!=end; ++tIt) {
        QListWidgetItem* li = new QListWidgetItem(QStringLiteral("%1 [ %2 ]").arg((*tIt).first,
                                                                           (*tIt).second));
        li->setData(Qt::UserRole,(*tIt).first);
        li->setData(Qt::UserRole+1,(*tIt).second);
        dlg->listDynAlbums->addItem(li);
    }
    dlg->setIgnoredFiles(db->sqlGetIgnoredFiles());
    dlg->setSearchEngines(ctxSearchEngines);
    dlg->updateSQLFields(false);

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
        scrollFactor=dlg->spinScrollFactor->value();
        searchScrollFactor=dlg->spinSearchScrollFactor->value();
        backgroundColor=dlg->getBkColor();
        frameColor=dlg->getFrameColor();
        idxFont=dlg->getIdxFont();
        ocrFont=dlg->getOCRFont();
        cachePixmaps=dlg->radioCachePixmaps->isChecked();
        useFineRendering=dlg->checkFineRendering->isChecked();
        filesystemWatcher=dlg->checkFSWatcher->isChecked();
        upscaleFilter=static_cast<Blitz::ScaleFilterType>(dlg->comboUpscaleFilter->currentIndex());
        downscaleFilter=static_cast<Blitz::ScaleFilterType>(dlg->comboDownscaleFilter->currentIndex());
        pdfRendering = static_cast<Z::PDFRendering>(dlg->comboPDFRendererMode->currentIndex());
        ctxSearchEngines=dlg->getSearchEngines();

#ifndef Q_OS_WIN
        tranSourceLang=dlg->getTranSourceLanguage();
        tranDestLang=dlg->getTranDestLanguage();
#endif

        if (dlg->checkForceDPI->isChecked())
            forceDPI = dlg->spinForceDPI->value();
        else
            forceDPI = -1.0;
        if (dlg->radioMySQL->isChecked())
            dbEngine = Z::MySQL;
        else if (dlg->radioSQLite->isChecked())
            dbEngine = Z::SQLite;
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
        if (w!=nullptr) {
            w->updateBookmarks();
            w->updateViewer();
            w->searchTab->setEnabled(dbEngine!=Z::UndefinedDB);
            if (w->searchTab->isEnabled())
                w->searchTab->updateAlbumsList();
        }
        emit dbCheckBase();
        emit dbSetIgnoredFiles(dlg->getIgnoredFiles());
        if (filesystemWatcher)
            emit dbRescanIndexedDirs();
        if (ocrEditor!=nullptr)
            ocrEditor->setEditorFont(ocrFont);

#ifdef WITH_OCR
        QSettings settingsOCR(QStringLiteral("kernel1024"), QStringLiteral("qmanga-ocr"));
        settingsOCR.beginGroup(QStringLiteral("Main"));
        settingsOCR.setValue(QStringLiteral("activeLanguage"), dlg->getOCRLanguage());
        settingsOCR.setValue(QStringLiteral("datapath"), dlg->editOCRDatapath->text());
        settingsOCR.endGroup();
#endif
    }
    dlg->setParent(nullptr);
    delete dlg;
}

QUrl ZGlobal::createSearchUrl(const QString& text, const QString& engine)
{
    if (ctxSearchEngines.isEmpty())
        return QUrl();

    auto it = ctxSearchEngines.constKeyValueBegin();
    QString url = (*it).second;
    if (engine.isEmpty() && !defaultSearchEngine.isEmpty())
        url = ctxSearchEngines.value(defaultSearchEngine);
    if (!engine.isEmpty() && ctxSearchEngines.contains(engine))
        url = ctxSearchEngines.value(engine);

    url.replace(QStringLiteral("%s"),text);
    url.replace(QStringLiteral("%ps"),QUrl::toPercentEncoding(text));

    return QUrl::fromUserInput(url);
}

void ZGlobal::initLanguagesList()
{
    QList<QLocale> allLocales = QLocale::matchingLocales(
                QLocale::AnyLanguage,
                QLocale::AnyScript,
                QLocale::AnyCountry);

    langNamesList.clear();
    langSortedBCP47List.clear();

    for(const QLocale &locale : allLocales) {
        QString bcp = locale.bcp47Name();
        QString name = QStringLiteral("%1 (%2)").arg(QLocale::languageToString(locale.language()),bcp);

        // filter out unsupported codes for dialects
        if (bcp.contains('-') && !bcp.startsWith(QStringLiteral("zh"))) continue;

        // replace C locale with English
        if (bcp == QStringLiteral("en"))
            name = QStringLiteral("English (en)");

        if (!langNamesList.contains(bcp)) {
            langNamesList.insert(bcp,name);
            langSortedBCP47List.insert(name,bcp);
        }
    }
}

QString ZGlobal::getLanguageName(const QString& bcp47Name)
{
    return langNamesList.value(bcp47Name);
}

QStringList ZGlobal::getLanguageCodes() const
{
    return langSortedBCP47List.values();
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

ZFileEntry::ZFileEntry(const QString &aName, int aIdx)
{
    name = aName;
    idx = aIdx;
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
