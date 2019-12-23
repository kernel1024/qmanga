#include <QMessageBox>
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
    initLanguagesList();
    tranSourceLang = QSL("ja");
    tranDestLang = QSL("en");

    fsWatcher = new QFileSystemWatcher(this);
    connect(fsWatcher,&QFileSystemWatcher::directoryChanged,this,&ZGlobal::directoryChanged);

    threadDB = new QThread();
    db = new ZDB();
    connect(this,&ZGlobal::dbSetCredentials,
            db,&ZDB::setCredentials,Qt::QueuedConnection);
    connect(this,&ZGlobal::dbCheckBase,db,&ZDB::sqlCheckBase,Qt::QueuedConnection);
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

    Q_EMIT auxMessage(tr("%1 problems with MySQL").arg(problems.count()));

    QString text;
    for (auto it=problems.constKeyValueBegin(), end=problems.constKeyValueEnd(); it!=end; ++it) {
        text.append(QSL("%1\n-------------------------------\n%2\n\n")
                    .arg((*it).first,(*it).second));
    }

    QMessageBox mbox;
    mbox.setParent(parent,Qt::Dialog);
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
    auto w = qobject_cast<ZMainWindow *>(parent());

    QSettings settings(QSL("kernel1024"), QSL("qmanga"));
    settings.beginGroup(QSL("MainWindow"));
    cacheWidth = settings.value(QSL("cacheWidth"),ZDefaults::cacheWidth).toInt();
    downscaleFilter = static_cast<Blitz::ScaleFilterType>(
                          settings.value(QSL("downscaleFilter"),Blitz::LanczosFilter).toInt());
    upscaleFilter = static_cast<Blitz::ScaleFilterType>(
                        settings.value(QSL("upscaleFilter"),Blitz::MitchellFilter).toInt());
    dbUser = settings.value(QSL("mysqlUser"),QString()).toString();
    dbPass = settings.value(QSL("mysqlPassword"),QString()).toString();
    dbBase = settings.value(QSL("mysqlBase"),QSL("qmanga")).toString();
    dbHost = settings.value(QSL("mysqlHost"),QSL("localhost")).toString();
    rarCmd = settings.value(QSL("rarCmd"),QString()).toString();
    savedAuxOpenDir = settings.value(QSL("savedAuxOpenDir"),QString()).toString();
    savedIndexOpenDir = settings.value(QSL("savedIndexOpenDir"),QString()).toString();
    savedAuxSaveDir = settings.value(QSL("savedAuxSaveDir"),QString()).toString();
    magnifySize = settings.value(QSL("magnifySize"),ZDefaults::magnifySize).toInt();
    resizeBlur = settings.value(QSL("resizingBlur"),ZDefaults::resizeBlur).toDouble();
    scrollDelta = settings.value(QSL("scrollDelta"),ZDefaults::scrollDelta).toInt();
    scrollFactor = settings.value(QSL("scrollFactor"),ZDefaults::scrollFactor).toInt();
    searchScrollFactor = settings.value(QSL("searchScrollFactor"),ZDefaults::searchScrollFactor).toDouble();
    pdfRendering = static_cast<Z::PDFRendering>(
                       settings.value(QSL("pdfRendering"),
                                      Z::Autodetect).toInt());
    dbEngine = static_cast<Z::DBMS>(settings.value(QSL("dbEngine"),Z::SQLite).toInt());
    forceDPI = settings.value(QSL("forceDPI"),ZDefaults::forceDPI).toDouble();
    backgroundColor = QColor(settings.value(QSL("backgroundColor"),
                                            QSL("#303030")).toString());
    frameColor = QColor(settings.value(QSL("frameColor"),
                                       QColor(Qt::lightGray).name()).toString());
    cachePixmaps = settings.value(QSL("cachePixmaps"),false).toBool();
    useFineRendering = settings.value(QSL("fineRendering"),true).toBool();
    filesystemWatcher = settings.value(QSL("filesystemWatcher"),false).toBool();
    defaultSearchEngine = settings.value(QSL("defaultSearchEngine"),QString()).toString();
    ctxSearchEngines = settings.value(QSL("ctxSearchEngines")).value<ZStrMap>();


    if (!idxFont.fromString(settings.value(QSL("idxFont"),QString()).toString()))
        idxFont = QApplication::font("QListView");
    if (!ocrFont.fromString(settings.value(QSL("ocrFont"),QString()).toString()))
        ocrFont = QApplication::font("QPlainTextView");
    if (!backgroundColor.isValid())
        backgroundColor = QApplication::palette("QWidget").dark().color();
    if (!frameColor.isValid())
        frameColor = QColor(Qt::lightGray);

    bool showMaximized = false;
    if (w!=nullptr)
        showMaximized = settings.value(QSL("maximized"),false).toBool();

    bookmarks = settings.value(QSL("bookmarks")).value<ZStrMap>();
    ZStrMap albums = settings.value(QSL("dynAlbums")).value<ZStrMap>();
    Q_EMIT dbSetDynAlbums(albums);

    settings.endGroup();

    Q_EMIT dbSetCredentials(dbHost,dbBase,dbUser,dbPass);
    Q_EMIT loadSearchTabSettings(&settings);

    if (w!=nullptr) {
        if (showMaximized)
            w->showMaximized();

        w->updateBookmarks();
        w->updateViewer();
    }
    Q_EMIT dbCheckBase();
    if (filesystemWatcher)
        Q_EMIT dbRescanIndexedDirs();

    if (ocrEditor!=nullptr)
        ocrEditor->setEditorFont(ocrFont);

    checkSQLProblems(w);
}

void ZGlobal::saveSettings()
{
    QSettings settings(QSL("kernel1024"), QSL("qmanga"));
    settings.beginGroup(QSL("MainWindow"));
    settings.remove(QString());
    settings.setValue(QSL("cacheWidth"),cacheWidth);
    settings.setValue(QSL("downscaleFilter"),static_cast<int>(downscaleFilter));
    settings.setValue(QSL("upscaleFilter"),static_cast<int>(upscaleFilter));
    settings.setValue(QSL("mysqlUser"),dbUser);
    settings.setValue(QSL("mysqlPassword"),dbPass);
    settings.setValue(QSL("mysqlBase"),dbBase);
    settings.setValue(QSL("mysqlHost"),dbHost);
    settings.setValue(QSL("rarCmd"),rarCmd);
    settings.setValue(QSL("savedAuxOpenDir"),savedAuxOpenDir);
    settings.setValue(QSL("savedAuxSaveDir"),savedAuxSaveDir);
    settings.setValue(QSL("savedIndexOpenDir"),savedIndexOpenDir);
    settings.setValue(QSL("magnifySize"),magnifySize);
    settings.setValue(QSL("resizingBlur"),resizeBlur);
    settings.setValue(QSL("scrollDelta"),scrollDelta);
    settings.setValue(QSL("scrollFactor"),scrollFactor);
    settings.setValue(QSL("searchScrollFactor"),searchScrollFactor);
    settings.setValue(QSL("pdfRendering"),pdfRendering);
    settings.setValue(QSL("dbEngine"),dbEngine);
    settings.setValue(QSL("forceDPI"),forceDPI);
    settings.setValue(QSL("backgroundColor"),backgroundColor.name());
    settings.setValue(QSL("frameColor"),frameColor.name());
    settings.setValue(QSL("cachePixmaps"),cachePixmaps);
    settings.setValue(QSL("fineRendering"),useFineRendering);
    settings.setValue(QSL("filesystemWatcher"),filesystemWatcher);
    settings.setValue(QSL("idxFont"),idxFont.toString());
    settings.setValue(QSL("ocrFont"),ocrFont.toString());
    settings.setValue(QSL("defaultSearchEngine"),defaultSearchEngine);
    settings.setValue(QSL("ctxSearchEngines"),QVariant::fromValue(ctxSearchEngines));
    settings.setValue(QSL("tranSourceLanguage"),tranSourceLang);
    settings.setValue(QSL("tranDestLanguage"),tranDestLang);

    auto w = qobject_cast<ZMainWindow *>(parent());
    if (w!=nullptr) {
        settings.setValue(QSL("maximized"),w->isMaximized());

    }

    settings.setValue(QSL("bookmarks"),QVariant::fromValue(bookmarks));
    settings.setValue(QSL("dynAlbums"),QVariant::fromValue(db->getDynAlbums()));

    settings.endGroup();

    Q_EMIT saveSearchTabSettings(&settings);
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
    auto w = qobject_cast<ZMainWindow *>(parent());
    checkSQLProblems(w);
}

void ZGlobal::addFineRenderTime(qint64 msec)
{
    fineRenderMutex.lock();

    fineRenderTimes.append(msec);
    if (fineRenderTimes.count()>100)
        fineRenderTimes.removeFirst();

    qint64 sum = 0;
    for (const qint64 a : qAsConst(fineRenderTimes))
        sum += a;
    avgFineRenderTime = static_cast<int>(sum) / fineRenderTimes.count();

    fineRenderMutex.unlock();
}

QColor ZGlobal::foregroundColor()
{
    constexpr qreal redLuma = 0.2989;
    constexpr qreal greenLuma = 0.5870;
    constexpr qreal blueLuma = 0.1140;
    constexpr qreal halfLuma = 0.5;
    qreal r;
    qreal g;
    qreal b;
    backgroundColor.getRgbF(&r,&g,&b);
    qreal br = r*redLuma+g*greenLuma+b*blueLuma;
    if (br>halfLuma)
        return QColor(Qt::black);

    return QColor(Qt::white);
}

void ZGlobal::fsCheckFilesAvailability()
{
    int i = 0;
    while (i<newlyAddedFiles.count() && !newlyAddedFiles.isEmpty()) {
        QFileInfo fi(newlyAddedFiles.at(i));
        if (!(fi.isReadable() && fi.isFile())) {
            newlyAddedFiles.removeAt(i);
        } else {
            i++;
        }
    }
    Q_EMIT fsFilesAdded();
}

void ZGlobal::settingsDlg()
{
    auto w = qobject_cast<ZMainWindow *>(parent());
    auto dlg = new ZSettingsDialog(w);

    checkSQLProblems(w);

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
    if (cachePixmaps) {
        dlg->radioCachePixmaps->setChecked(true);
    } else {
        dlg->radioCacheData->setChecked(true);
    }
    if (dbEngine==Z::MySQL) {
        dlg->radioMySQL->setChecked(true);
    } else {
        dlg->radioSQLite->setChecked(true);
    }
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
    if (forceDPI<=0.0) {
        dlg->spinForceDPI->setValue(dpiX);
    } else {
        dlg->spinForceDPI->setValue(forceDPI);
    }

    for (auto bIt=bookmarks.constKeyValueBegin(), end=bookmarks.constKeyValueEnd(); bIt!=end; ++bIt) {
        QString st = (*bIt).second;
        if (st.split('\n').count()>0)
            st = st.split('\n').at(0);
        auto li = new QListWidgetItem(QSL("%1 [ %2 ]").arg((*bIt).first,
                                                                           (*bIt).second));
        li->setData(Qt::UserRole,(*bIt).first);
        li->setData(Qt::UserRole+1,(*bIt).second);
        dlg->listBookmarks->addItem(li);
    }
    ZStrMap albums = db->getDynAlbums();
    for (auto tIt = albums.constKeyValueBegin(), end = albums.constKeyValueEnd(); tIt!=end; ++tIt) {
        auto li = new QListWidgetItem(QSL("%1 [ %2 ]").arg((*tIt).first,
                                                                           (*tIt).second));
        li->setData(Qt::UserRole,(*tIt).first);
        li->setData(Qt::UserRole+1,(*tIt).second);
        dlg->listDynAlbums->addItem(li);
    }
    dlg->setIgnoredFiles(db->sqlGetIgnoredFiles());
    dlg->setSearchEngines(ctxSearchEngines);
    dlg->updateSQLFields(false);

#ifdef WITH_OCR
    dlg->editOCRDatapath->setText(ocrGetDatapath());
    dlg->updateOCRLanguages();
#endif

    if (dlg->exec()==QDialog::Accepted) {
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

        if (dlg->checkForceDPI->isChecked()) {
            forceDPI = dlg->spinForceDPI->value();
        } else {
            forceDPI = -1.0;
        }
        if (dlg->radioMySQL->isChecked()) {
            dbEngine = Z::MySQL;
        } else if (dlg->radioSQLite->isChecked()) {
            dbEngine = Z::SQLite;
        }
        bookmarks.clear();
        for (int i=0; i<dlg->listBookmarks->count(); i++) {
            bookmarks[dlg->listBookmarks->item(i)->data(Qt::UserRole).toString()]=
                    dlg->listBookmarks->item(i)->data(Qt::UserRole+1).toString();
        }
        Q_EMIT dbSetCredentials(dbHost,dbBase,dbUser,dbPass);
        albums.clear();
        for (int i=0; i<dlg->listDynAlbums->count(); i++) {
            albums[dlg->listDynAlbums->item(i)->data(Qt::UserRole).toString()]=
                    dlg->listDynAlbums->item(i)->data(Qt::UserRole+1).toString();
        }
        Q_EMIT dbSetDynAlbums(albums);
        if (w!=nullptr) {
            w->updateBookmarks();
            w->updateViewer();
        }
        Q_EMIT loadSearchTabSettings(nullptr);
        Q_EMIT dbCheckBase();
        Q_EMIT dbSetIgnoredFiles(dlg->getIgnoredFiles());
        if (filesystemWatcher)
            Q_EMIT dbRescanIndexedDirs();
        if (ocrEditor!=nullptr)
            ocrEditor->setEditorFont(ocrFont);

#ifdef WITH_OCR
        bool needRestart = (dlg->editOCRDatapath->text() != ocrGetDatapath());
        QSettings settingsOCR(QSL("kernel1024"), QSL("qmanga-ocr"));
        settingsOCR.beginGroup(QSL("Main"));
        settingsOCR.setValue(QSL("activeLanguage"), dlg->getOCRLanguage());
        settingsOCR.setValue(QSL("datapath"), dlg->editOCRDatapath->text());
        settingsOCR.endGroup();

        if (needRestart) {
            QMessageBox::information(nullptr,tr("QManga"),tr("OCR datapath changed.\n"
                                                             "The application needs to be restarted to apply these settings."));
        }
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

    url.replace(QSL("%s"),text);
    url.replace(QSL("%ps"),QUrl::toPercentEncoding(text));

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
        QString name = QSL("%1 (%2)").arg(QLocale::languageToString(locale.language()),bcp);

        // filter out unsupported codes for dialects
        if (bcp.contains('-') && !bcp.startsWith(QSL("zh"))) continue;

        // replace C locale with English
        if (bcp == QSL("en"))
            name = QSL("English (en)");

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

qint64 ZGlobal::getAvgFineRenderTime()
{
    return avgFineRenderTime;
}

QStringList ZGlobal::getLanguageCodes() const
{
    return langSortedBCP47List.values();
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
