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
#include "zglobalprivate.h"
#include "ui_settingsdialog.h"

ZGlobal::ZGlobal(QObject *parent) :
    QObject(parent),
    dptr(new ZGlobalPrivate(this))
{
    Q_D(ZGlobal);

    initLanguagesList();
    d->m_tranSourceLang = QSL("ja");
    d->m_tranDestLang = QSL("en");

    connect(d->fsWatcher.data(),&QFileSystemWatcher::directoryChanged,this,&ZGlobal::directoryChanged);

    connect(this,&ZGlobal::dbSetCredentials,
            d->m_db.data(),&ZDB::setCredentials,Qt::QueuedConnection);
    connect(this,&ZGlobal::dbCheckBase,d->m_db.data(),&ZDB::sqlCheckBase,Qt::QueuedConnection);
    connect(this,&ZGlobal::dbRescanIndexedDirs,d->m_db.data(),&ZDB::sqlRescanIndexedDirs,Qt::QueuedConnection);
    connect(d->m_db.data(),&ZDB::updateWatchDirList,
            this,&ZGlobal::updateWatchDirList,Qt::QueuedConnection);
    connect(d->m_db.data(),&ZDB::baseCheckComplete,
            this,&ZGlobal::dbCheckComplete,Qt::QueuedConnection);
    connect(this,&ZGlobal::dbSetDynAlbums,d->m_db.data(),&ZDB::setDynAlbums,Qt::QueuedConnection);
    connect(this,&ZGlobal::dbSetIgnoredFiles,
            d->m_db.data(),&ZDB::sqlSetIgnoredFiles,Qt::QueuedConnection);
    d->m_db->moveToThread(d->m_threadDB.data());
    connect(d->m_threadDB.data(),&QThread::finished,d->m_db.data(),&QObject::deleteLater,Qt::QueuedConnection);
    connect(d->m_threadDB.data(),&QThread::finished,d->m_threadDB.data(),&QObject::deleteLater,Qt::QueuedConnection);

    d->m_threadDB->start();
}

ZGlobal::~ZGlobal()
{
    Q_D(ZGlobal);
    d->m_threadDB->quit();
    QApplication::processEvents();
}

void ZGlobal::checkSQLProblems(QWidget *parent)
{
    Q_D(const ZGlobal);
    ZStrHash problems = d->m_db->getConfigProblems();
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
    Q_D(ZGlobal);
    if (!d->m_mainWindow) return;

    QSettings settings(QSL("kernel1024"), QSL("qmanga"));
    settings.beginGroup(QSL("MainWindow"));
    d->m_cacheWidth = settings.value(QSL("cacheWidth"),ZDefaults::cacheWidth).toInt();
    d->m_downscaleFilter = static_cast<Blitz::ScaleFilterType>(
                          settings.value(QSL("downscaleFilter"),Blitz::LanczosFilter).toInt());
    d->m_upscaleFilter = static_cast<Blitz::ScaleFilterType>(
                        settings.value(QSL("upscaleFilter"),Blitz::MitchellFilter).toInt());
    d->m_dbUser = settings.value(QSL("mysqlUser"),QString()).toString();
    d->m_dbPass = settings.value(QSL("mysqlPassword"),QString()).toString();
    d->m_dbBase = settings.value(QSL("mysqlBase"),QSL("qmanga")).toString();
    d->m_dbHost = settings.value(QSL("mysqlHost"),QSL("localhost")).toString();
    d->m_rarCmd = settings.value(QSL("rarCmd"),QString()).toString();
    d->m_savedAuxOpenDir = settings.value(QSL("savedAuxOpenDir"),QString()).toString();
    d->m_savedIndexOpenDir = settings.value(QSL("savedIndexOpenDir"),QString()).toString();
    d->m_savedAuxSaveDir = settings.value(QSL("savedAuxSaveDir"),QString()).toString();
    d->m_magnifySize = settings.value(QSL("magnifySize"),ZDefaults::magnifySize).toInt();
    d->m_resizeBlur = settings.value(QSL("resizingBlur"),ZDefaults::resizeBlur).toDouble();
    d->m_scrollDelta = settings.value(QSL("scrollDelta"),ZDefaults::scrollDelta).toInt();
    d->m_scrollFactor = settings.value(QSL("scrollFactor"),ZDefaults::scrollFactor).toInt();
    d->m_searchScrollFactor = settings.value(QSL("searchScrollFactor"),ZDefaults::searchScrollFactor).toDouble();
    d->m_pdfRendering = static_cast<Z::PDFRendering>(
                       settings.value(QSL("pdfRendering"),
                                      Z::pdfAutodetect).toInt());
    d->m_dbEngine = static_cast<Z::DBMS>(settings.value(QSL("dbEngine"),Z::dbmsSQLite).toInt());
    d->m_forceDPI = settings.value(QSL("forceDPI"),ZDefaults::forceDPI).toDouble();
    d->m_backgroundColor = QColor(settings.value(QSL("backgroundColor"),
                                            QSL("#303030")).toString());
    d->m_frameColor = QColor(settings.value(QSL("frameColor"),
                                       QColor(Qt::lightGray).name()).toString());
    d->m_cachePixmaps = settings.value(QSL("cachePixmaps"),false).toBool();
    d->m_useFineRendering = settings.value(QSL("fineRendering"),true).toBool();
    d->m_filesystemWatcher = settings.value(QSL("filesystemWatcher"),false).toBool();
    d->m_defaultSearchEngine = settings.value(QSL("defaultSearchEngine"),QString()).toString();
    d->m_ctxSearchEngines = settings.value(QSL("ctxSearchEngines")).value<ZStrMap>();

    if (!d->m_idxFont.fromString(settings.value(QSL("idxFont"),QString()).toString()))
        d->m_idxFont = QApplication::font("QListView");
    if (!d->m_ocrFont.fromString(settings.value(QSL("ocrFont"),QString()).toString()))
        d->m_ocrFont = QApplication::font("QPlainTextView");
    if (!d->m_backgroundColor.isValid())
        d->m_backgroundColor = QApplication::palette("QWidget").dark().color();
    if (!d->m_frameColor.isValid())
        d->m_frameColor = QColor(Qt::lightGray);

    bool showMaximized = settings.value(QSL("maximized"),false).toBool();

    d->m_bookmarks = settings.value(QSL("bookmarks")).value<ZStrMap>();
    ZStrMap albums = settings.value(QSL("dynAlbums")).value<ZStrMap>();
    Q_EMIT dbSetDynAlbums(albums);

    settings.endGroup();

    Q_EMIT dbSetCredentials(d->m_dbHost,d->m_dbBase,d->m_dbUser,d->m_dbPass);
    Q_EMIT loadSearchTabSettings(&settings);

    if (showMaximized)
        d->m_mainWindow->showMaximized();

    d->m_mainWindow->updateBookmarks();
    d->m_mainWindow->updateViewer();
    Q_EMIT dbCheckBase();
    if (d->m_filesystemWatcher)
        Q_EMIT dbRescanIndexedDirs();

    ocrEditor()->setEditorFont(d->m_ocrFont);

    checkSQLProblems(d->m_mainWindow);
}

void ZGlobal::saveSettings()
{
    Q_D(const ZGlobal);
    QSettings settings(QSL("kernel1024"), QSL("qmanga"));
    settings.beginGroup(QSL("MainWindow"));
    settings.remove(QString());
    settings.setValue(QSL("cacheWidth"),d->m_cacheWidth);
    settings.setValue(QSL("downscaleFilter"),static_cast<int>(d->m_downscaleFilter));
    settings.setValue(QSL("upscaleFilter"),static_cast<int>(d->m_upscaleFilter));
    settings.setValue(QSL("mysqlUser"),d->m_dbUser);
    settings.setValue(QSL("mysqlPassword"),d->m_dbPass);
    settings.setValue(QSL("mysqlBase"),d->m_dbBase);
    settings.setValue(QSL("mysqlHost"),d->m_dbHost);
    settings.setValue(QSL("rarCmd"),d->m_rarCmd);
    settings.setValue(QSL("savedAuxOpenDir"),d->m_savedAuxOpenDir);
    settings.setValue(QSL("savedAuxSaveDir"),d->m_savedAuxSaveDir);
    settings.setValue(QSL("savedIndexOpenDir"),d->m_savedIndexOpenDir);
    settings.setValue(QSL("magnifySize"),d->m_magnifySize);
    settings.setValue(QSL("resizingBlur"),d->m_resizeBlur);
    settings.setValue(QSL("scrollDelta"),d->m_scrollDelta);
    settings.setValue(QSL("scrollFactor"),d->m_scrollFactor);
    settings.setValue(QSL("searchScrollFactor"),d->m_searchScrollFactor);
    settings.setValue(QSL("pdfRendering"),d->m_pdfRendering);
    settings.setValue(QSL("dbEngine"),d->m_dbEngine);
    settings.setValue(QSL("forceDPI"),d->m_forceDPI);
    settings.setValue(QSL("backgroundColor"),d->m_backgroundColor.name());
    settings.setValue(QSL("frameColor"),d->m_frameColor.name());
    settings.setValue(QSL("cachePixmaps"),d->m_cachePixmaps);
    settings.setValue(QSL("fineRendering"),d->m_useFineRendering);
    settings.setValue(QSL("filesystemWatcher"),d->m_filesystemWatcher);
    settings.setValue(QSL("idxFont"),d->m_idxFont.toString());
    settings.setValue(QSL("ocrFont"),d->m_ocrFont.toString());
    settings.setValue(QSL("defaultSearchEngine"),d->m_defaultSearchEngine);
    settings.setValue(QSL("ctxSearchEngines"),QVariant::fromValue(d->m_ctxSearchEngines));
    settings.setValue(QSL("tranSourceLanguage"),d->m_tranSourceLang);
    settings.setValue(QSL("tranDestLanguage"),d->m_tranDestLang);

    if (d->m_mainWindow)
        settings.setValue(QSL("maximized"),d->m_mainWindow->isMaximized());

    settings.setValue(QSL("bookmarks"),QVariant::fromValue(d->m_bookmarks));
    settings.setValue(QSL("dynAlbums"),QVariant::fromValue(d->m_db->getDynAlbums()));

    settings.endGroup();

    Q_EMIT saveSearchTabSettings(&settings);
}

void ZGlobal::updateWatchDirList(const QStringList &watchDirs)
{
    Q_D(ZGlobal);
    d->m_dirWatchList.clear();
    for (const auto &i : watchDirs) {
        QDir dir(i);
        if (!dir.isReadable()) continue;
        d->m_dirWatchList[dir.absolutePath()] =
                dir.entryList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
    }
    d->fsWatcher->addPaths(watchDirs);
}

void ZGlobal::directoryChanged(const QString &dir)
{
    Q_D(ZGlobal);
    QStringList ignoredFiles;
    if (d->m_db!=nullptr)
        ignoredFiles = d->m_db->sqlGetIgnoredFiles();

    QDir dr(dir);
    if (!dr.isReadable()) return;
    const QStringList nlist = dr.entryList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
    const QStringList olist = d->m_dirWatchList.value(dr.absolutePath());
    for (const auto &i : nlist) {
        QString fname = dr.absoluteFilePath(i);
        if (!olist.contains(i) && !d->m_newlyAddedFiles.contains(fname) &&
                !ignoredFiles.contains(fname))
            d->m_newlyAddedFiles.append(fname);
    }
    fsCheckFilesAvailability();
}

void ZGlobal::dbCheckComplete()
{
    Q_D(const ZGlobal);
    checkSQLProblems(d->m_mainWindow);
}

void ZGlobal::addFineRenderTime(qint64 msec)
{
    Q_D(ZGlobal);
    d->m_fineRenderMutex.lock();

    d->m_fineRenderTimes.append(msec);
    if (d->m_fineRenderTimes.count()>100)
        d->m_fineRenderTimes.removeFirst();

    qint64 sum = 0;
    for (const qint64 a : qAsConst(d->m_fineRenderTimes))
        sum += a;
    d->m_avgFineRenderTime = static_cast<int>(sum) / d->m_fineRenderTimes.count();

    d->m_fineRenderMutex.unlock();
}

QColor ZGlobal::getForegroundColor() const
{
    Q_D(const ZGlobal);
    constexpr qreal redLuma = 0.2989;
    constexpr qreal greenLuma = 0.5870;
    constexpr qreal blueLuma = 0.1140;
    constexpr qreal halfLuma = 0.5;
    qreal r = 0.0;
    qreal g = 0.0;
    qreal b = 0.0;
    d->m_backgroundColor.getRgbF(&r,&g,&b);
    qreal br = r*redLuma+g*greenLuma+b*blueLuma;
    if (br>halfLuma)
        return QColor(Qt::black);

    return QColor(Qt::white);
}

void ZGlobal::fsCheckFilesAvailability()
{
    Q_D(ZGlobal);
    int i = 0;
    while (i<d->m_newlyAddedFiles.count() && !d->m_newlyAddedFiles.isEmpty()) {
        QFileInfo fi(d->m_newlyAddedFiles.at(i));
        if (!(fi.isReadable() && fi.isFile())) {
            d->m_newlyAddedFiles.removeAt(i);
        } else {
            i++;
        }
    }
    Q_EMIT fsFilesAdded();
}

void ZGlobal::setMainWindow(ZMainWindow *wnd)
{
    Q_D(ZGlobal);
    d->m_mainWindow = wnd;
}

void ZGlobal::settingsDlg()
{
    Q_D(ZGlobal);
    if (!d->m_mainWindow) return;

    ZSettingsDialog dlg(d->m_mainWindow);

    checkSQLProblems(d->m_mainWindow);

    dlg.ui->editMySqlLogin->setText(d->m_dbUser);
    dlg.ui->editMySqlPassword->setText(d->m_dbPass);
    dlg.ui->editMySqlBase->setText(d->m_dbBase);
    dlg.ui->editMySqlHost->setText(d->m_dbHost);
    dlg.ui->editRar->setText(d->m_rarCmd);
    dlg.ui->spinCacheWidth->setValue(d->m_cacheWidth);
    dlg.ui->spinMagnify->setValue(d->m_magnifySize);
    dlg.ui->spinBlur->setValue(d->m_resizeBlur);
    dlg.ui->spinScrollDelta->setValue(d->m_scrollDelta);
    dlg.ui->spinScrollFactor->setValue(d->m_scrollFactor);
    dlg.ui->spinSearchScrollFactor->setValue(d->m_searchScrollFactor);
    dlg.ui->labelDetectedDelta->setText(tr("Detected delta per one scroll event: %1 deg").arg(d->m_detectedDelta));
    if (d->m_cachePixmaps) {
        dlg.ui->rbCachePixmaps->setChecked(true);
    } else {
        dlg.ui->rbCacheData->setChecked(true);
    }
    if (d->m_dbEngine==Z::dbmsMySQL) {
        dlg.ui->rbMySQL->setChecked(true);
    } else {
        dlg.ui->rbSQLite->setChecked(true);
    }
    dlg.ui->checkFineRendering->setChecked(d->m_useFineRendering);
    dlg.updateBkColor(d->m_backgroundColor);
    dlg.updateIdxFont(d->m_idxFont);
    dlg.updateOCRFont(d->m_ocrFont);
    dlg.updateFrameColor(d->m_frameColor);
    dlg.ui->checkFSWatcher->setChecked(d->m_filesystemWatcher);
    dlg.ui->comboUpscaleFilter->setCurrentIndex(static_cast<int>(d->m_upscaleFilter));
    dlg.ui->comboDownscaleFilter->setCurrentIndex(static_cast<int>(d->m_downscaleFilter));
    dlg.ui->comboPDFRenderMode->setCurrentIndex(static_cast<int>(d->m_pdfRendering));
    dlg.ui->spinPDFDPI->setEnabled(d->m_forceDPI>0.0);
    dlg.ui->checkPDFDPI->setChecked(d->m_forceDPI>0.0);
    if (d->m_forceDPI<=0.0) {
        dlg.ui->spinPDFDPI->setValue(d->m_dpiX);
    } else {
        dlg.ui->spinPDFDPI->setValue(d->m_forceDPI);
    }

    for (auto bIt=d->m_bookmarks.constKeyValueBegin(), end=d->m_bookmarks.constKeyValueEnd(); bIt!=end; ++bIt) {
        QString st = (*bIt).second;
        if (st.split('\n').count()>0)
            st = st.split('\n').at(0);
        auto *li = new QListWidgetItem(QSL("%1 [ %2 ]").arg((*bIt).first,
                                                           (*bIt).second));
        li->setData(Qt::UserRole,(*bIt).first);
        li->setData(Qt::UserRole+1,(*bIt).second);
        dlg.ui->listBookmarks->addItem(li);
    }
    ZStrMap albums = d->m_db->getDynAlbums();
    for (auto tIt = albums.constKeyValueBegin(), end = albums.constKeyValueEnd(); tIt!=end; ++tIt) {
        auto *li = new QListWidgetItem(QSL("%1 [ %2 ]").arg((*tIt).first,
                                                           (*tIt).second));
        li->setData(Qt::UserRole,(*tIt).first);
        li->setData(Qt::UserRole+1,(*tIt).second);
        dlg.ui->listDynAlbums->addItem(li);
    }
    dlg.setIgnoredFiles(d->m_db->sqlGetIgnoredFiles());
    dlg.setSearchEngines(d->m_ctxSearchEngines);
    dlg.updateSQLFields(false);

#ifdef WITH_OCR
    dlg.ui->editOCRDatapath->setText(zF->ocrGetDatapath());
    dlg.updateOCRLanguages();
#endif

    if (dlg.exec()==QDialog::Accepted) {
        d->m_dbUser=dlg.ui->editMySqlLogin->text();
        d->m_dbPass=dlg.ui->editMySqlPassword->text();
        d->m_dbBase=dlg.ui->editMySqlBase->text();
        d->m_dbHost=dlg.ui->editMySqlHost->text();
        d->m_rarCmd=dlg.ui->editRar->text();
        d->m_cacheWidth=dlg.ui->spinCacheWidth->value();
        d->m_magnifySize=dlg.ui->spinMagnify->value();
        d->m_resizeBlur=dlg.ui->spinBlur->value();
        d->m_scrollDelta=dlg.ui->spinScrollDelta->value();
        d->m_scrollFactor=dlg.ui->spinScrollFactor->value();
        d->m_searchScrollFactor=dlg.ui->spinSearchScrollFactor->value();
        d->m_backgroundColor=dlg.getBkColor();
        d->m_frameColor=dlg.getFrameColor();
        d->m_idxFont=dlg.getIdxFont();
        d->m_ocrFont=dlg.getOCRFont();
        d->m_cachePixmaps=dlg.ui->rbCachePixmaps->isChecked();
        d->m_useFineRendering=dlg.ui->checkFineRendering->isChecked();
        d->m_filesystemWatcher=dlg.ui->checkFSWatcher->isChecked();
        d->m_upscaleFilter=static_cast<Blitz::ScaleFilterType>(dlg.ui->comboUpscaleFilter->currentIndex());
        d->m_downscaleFilter=static_cast<Blitz::ScaleFilterType>(dlg.ui->comboDownscaleFilter->currentIndex());
        d->m_pdfRendering = static_cast<Z::PDFRendering>(dlg.ui->comboPDFRenderMode->currentIndex());
        d->m_ctxSearchEngines=dlg.getSearchEngines();

#ifndef Q_OS_WIN
        d->m_tranSourceLang=dlg.getTranSourceLanguage();
        d->m_tranDestLang=dlg.getTranDestLanguage();
#endif

        if (dlg.ui->checkPDFDPI->isChecked()) {
            d->m_forceDPI = dlg.ui->spinPDFDPI->value();
        } else {
            d->m_forceDPI = -1.0;
        }
        if (dlg.ui->rbMySQL->isChecked()) {
            d->m_dbEngine = Z::dbmsMySQL;
        } else if (dlg.ui->rbSQLite->isChecked()) {
            d->m_dbEngine = Z::dbmsSQLite;
        }
        d->m_bookmarks.clear();
        for (int i=0; i<dlg.ui->listBookmarks->count(); i++) {
            d->m_bookmarks[dlg.ui->listBookmarks->item(i)->data(Qt::UserRole).toString()]=
                    dlg.ui->listBookmarks->item(i)->data(Qt::UserRole+1).toString();
        }
        Q_EMIT dbSetCredentials(d->m_dbHost,d->m_dbBase,d->m_dbUser,d->m_dbPass);
        albums.clear();
        for (int i=0; i<dlg.ui->listDynAlbums->count(); i++) {
            albums[dlg.ui->listDynAlbums->item(i)->data(Qt::UserRole).toString()]=
                    dlg.ui->listDynAlbums->item(i)->data(Qt::UserRole+1).toString();
        }
        Q_EMIT dbSetDynAlbums(albums);
        d->m_mainWindow->updateBookmarks();
        d->m_mainWindow->updateViewer();
        Q_EMIT loadSearchTabSettings(nullptr);
        Q_EMIT dbCheckBase();
        Q_EMIT dbSetIgnoredFiles(dlg.getIgnoredFiles());
        if (d->m_filesystemWatcher)
            Q_EMIT dbRescanIndexedDirs();
        ocrEditor()->setEditorFont(d->m_ocrFont);

#ifdef WITH_OCR
        bool needRestart = (dlg.ui->editOCRDatapath->text() != zF->ocrGetDatapath());
        QSettings settingsOCR(QSL("kernel1024"), QSL("qmanga-ocr"));
        settingsOCR.beginGroup(QSL("Main"));
        settingsOCR.setValue(QSL("activeLanguage"), dlg.getOCRLanguage());
        settingsOCR.setValue(QSL("datapath"), dlg.ui->editOCRDatapath->text());
        settingsOCR.endGroup();

        if (needRestart) {
            QMessageBox::information(nullptr,tr("QManga"),tr("OCR datapath changed.\n"
                                                             "The application needs to be restarted to apply these settings."));
        }
#endif
    }
}

QUrl ZGlobal::createSearchUrl(const QString& text, const QString& engine) const
{
    Q_D(const ZGlobal);
    if (d->m_ctxSearchEngines.isEmpty())
        return QUrl();

    auto it = d->m_ctxSearchEngines.constKeyValueBegin();
    QString url = (*it).second;
    if (engine.isEmpty() && !d->m_defaultSearchEngine.isEmpty())
        url = d->m_ctxSearchEngines.value(d->m_defaultSearchEngine);
    if (!engine.isEmpty() && d->m_ctxSearchEngines.contains(engine))
        url = d->m_ctxSearchEngines.value(engine);

    url.replace(QSL("%s"),text);
    url.replace(QSL("%ps"),QString::fromUtf8(QUrl::toPercentEncoding(text)));

    return QUrl::fromUserInput(url);
}

void ZGlobal::initLanguagesList()
{
    Q_D(ZGlobal);
    const QList<QLocale> allLocales = QLocale::matchingLocales(
                QLocale::AnyLanguage,
                QLocale::AnyScript,
                QLocale::AnyCountry);

    d->m_langNamesList.clear();
    d->m_langSortedBCP47List.clear();

    for(const QLocale &locale : allLocales) {
        QString bcp = locale.bcp47Name();
        QString name = QSL("%1 (%2)").arg(QLocale::languageToString(locale.language()),bcp);

        // filter out unsupported codes for dialects
        if (bcp.contains('-') && !bcp.startsWith(QSL("zh"))) continue;

        // replace C locale with English
        if (bcp == QSL("en"))
            name = QSL("English (en)");

        if (!d->m_langNamesList.contains(bcp)) {
            d->m_langNamesList.insert(bcp,name);
            d->m_langSortedBCP47List.insert(name,bcp);
        }
    }
}

double ZGlobal::getSearchScrollFactor() const
{
    Q_D(const ZGlobal);
    return d->m_searchScrollFactor;
}

double ZGlobal::getResizeBlur() const
{
    Q_D(const ZGlobal);
    return d->m_resizeBlur;
}

qreal ZGlobal::getForceDPI() const
{
    Q_D(const ZGlobal);
    return d->m_forceDPI;
}

qreal ZGlobal::getDpiY() const
{
    Q_D(const ZGlobal);
    return d->m_dpiY;
}

qreal ZGlobal::getDpiX() const
{
    Q_D(const ZGlobal);
    return d->m_dpiX;
}

int ZGlobal::getScrollFactor() const
{
    Q_D(const ZGlobal);
    return d->m_scrollFactor;
}

int ZGlobal::getDetectedDelta() const
{
    Q_D(const ZGlobal);
    return d->m_detectedDelta;
}

int ZGlobal::getScrollDelta() const
{
    Q_D(const ZGlobal);
    return d->m_scrollDelta;
}

int ZGlobal::getMagnifySize() const
{
    Q_D(const ZGlobal);
    return d->m_magnifySize;
}

int ZGlobal::getCacheWidth() const
{
    Q_D(const ZGlobal);
    return d->m_cacheWidth;
}

bool ZGlobal::getFilesystemWatcher() const
{
    Q_D(const ZGlobal);
    return d->m_filesystemWatcher;
}

bool ZGlobal::getUseFineRendering() const
{
    Q_D(const ZGlobal);
    return d->m_useFineRendering;
}

bool ZGlobal::getCachePixmaps() const
{
    Q_D(const ZGlobal);
    return d->m_cachePixmaps;
}

Z::DBMS ZGlobal::getDbEngine() const
{
    Q_D(const ZGlobal);
    return d->m_dbEngine;
}

Z::PDFRendering ZGlobal::getPdfRendering() const
{
    Q_D(const ZGlobal);
    return d->m_pdfRendering;
}

Blitz::ScaleFilterType ZGlobal::getUpscaleFilter() const
{
    Q_D(const ZGlobal);
    return d->m_upscaleFilter;
}

Blitz::ScaleFilterType ZGlobal::getDownscaleFilter() const
{
    Q_D(const ZGlobal);
    return d->m_downscaleFilter;
}

QString ZGlobal::getDefaultSearchEngine() const
{
    Q_D(const ZGlobal);
    return d->m_defaultSearchEngine;
}

QString ZGlobal::getTranSourceLang() const
{
    Q_D(const ZGlobal);
    return d->m_tranSourceLang;
}

QString ZGlobal::getTranDestLang() const
{
    Q_D(const ZGlobal);
    return d->m_tranDestLang;
}

QString ZGlobal::getSavedAuxOpenDir() const
{
    Q_D(const ZGlobal);
    return d->m_savedAuxOpenDir;
}

void ZGlobal::setSavedAuxOpenDir(const QString &value)
{
    Q_D(ZGlobal);
    d->m_savedAuxOpenDir = value;
}

QString ZGlobal::getSavedIndexOpenDir() const
{
    Q_D(const ZGlobal);
    return d->m_savedIndexOpenDir;
}

void ZGlobal::setSavedIndexOpenDir(const QString &value)
{
    Q_D(ZGlobal);
    d->m_savedIndexOpenDir = value;
}

QString ZGlobal::getSavedAuxSaveDir() const
{
    Q_D(const ZGlobal);
    return d->m_savedAuxSaveDir;
}

void ZGlobal::setSavedAuxSaveDir(const QString &value)
{
    Q_D(ZGlobal);
    d->m_savedAuxSaveDir = value;
}

QColor ZGlobal::getBackgroundColor() const
{
    Q_D(const ZGlobal);
    return d->m_backgroundColor;
}

QColor ZGlobal::getFrameColor() const
{
    Q_D(const ZGlobal);
    return d->m_frameColor;
}

QFont ZGlobal::getIdxFont() const
{
    Q_D(const ZGlobal);
    return d->m_idxFont;
}

QFont ZGlobal::getOcrFont() const
{
    Q_D(const ZGlobal);
    return d->m_ocrFont;
}

QString ZGlobal::getRarCmd() const
{
    Q_D(const ZGlobal);
    return d->m_rarCmd;
}

void ZGlobal::setDefaultSearchEngine(const QString &value)
{
    Q_D(ZGlobal);
    d->m_defaultSearchEngine = value;
}

QStringList ZGlobal::getNewlyAddedFiles() const
{
    Q_D(const ZGlobal);
    return d->m_newlyAddedFiles;
}

void ZGlobal::removeFileFromNewlyAdded(const QString &filename)
{
    Q_D(ZGlobal);
    d->m_newlyAddedFiles.removeAll(filename);
}

ZStrMap ZGlobal::getBookmarks() const
{
    Q_D(const ZGlobal);
    return d->m_bookmarks;
}

void ZGlobal::addBookmark(const QString &title, const QString &filename)
{
    Q_D(ZGlobal);
    d->m_bookmarks[title] = filename;
}

ZStrMap ZGlobal::getCtxSearchEngines() const
{
    Q_D(const ZGlobal);
    return d->m_ctxSearchEngines;
}

ZDB *ZGlobal::db() const
{
    Q_D(const ZGlobal);
    return d->m_db.data();
}

ZOCREditor *ZGlobal::ocrEditor()
{
    Q_D(ZGlobal);
    if (!d->m_ocrEditor)
        d->m_ocrEditor.reset(new ZOCREditor(d->m_mainWindow));

    return d->m_ocrEditor.data();
}

void ZGlobal::setDPI(qreal x, qreal y)
{
    Q_D(ZGlobal);
    d->m_dpiX = x;
    d->m_dpiY = y;
}

void ZGlobal::setDetectedDelta(int value)
{
    Q_D(ZGlobal);
    d->m_detectedDelta = value;
}

QString ZGlobal::getLanguageName(const QString& bcp47Name) const
{
    Q_D(const ZGlobal);
    return d->m_langNamesList.value(bcp47Name);
}

qint64 ZGlobal::getAvgFineRenderTime() const
{
    Q_D(const ZGlobal);
    return d->m_avgFineRenderTime;
}

QStringList ZGlobal::getLanguageCodes() const
{
    Q_D(const ZGlobal);
    return d->m_langSortedBCP47List.values();
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
        return (zF->compareWithNumerics(fi1.completeBaseName(),fi2.completeBaseName())<0);
    return (zF->compareWithNumerics(fi1.path(),fi2.path())<0);
}

bool ZFileEntry::operator >(const ZFileEntry &ref) const
{
    QFileInfo fi1(name);
    QFileInfo fi2(ref.name);
    if (fi1.path()==fi2.path())
        return (zF->compareWithNumerics(fi1.completeBaseName(),fi2.completeBaseName())>0);
    return (zF->compareWithNumerics(fi1.path(),fi2.path())>0);
}
