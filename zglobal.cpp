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
    cacheWidth = 5;
    bookmarks.clear();

    threadDB = new QThread();
    db = new ZDB();
    connect(this,SIGNAL(dbSetCredentials(QString,QString,QString)),
            db,SLOT(setCredentials(QString,QString,QString)),Qt::QueuedConnection);
    connect(this,SIGNAL(dbCheckBase()),db,SLOT(sqlCheckBase()),Qt::QueuedConnection);
    connect(this,SIGNAL(dbCheckEmptyAlbums()),db,SLOT(sqlDelEmptyAlbums()),Qt::QueuedConnection);
    db->moveToThread(threadDB);
    threadDB->start();

    int screen = 0;
    QDesktopWidget *desktop = QApplication::desktop();
    if (desktop->isVirtualDesktop()) {
        screen = desktop->screenNumber(QCursor::pos());
    }

    pdfRenderWidth = 2*desktop->screen(screen)->width()/3;
    if (pdfRenderWidth<1000)
        pdfRenderWidth = 1000;

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
    cacheWidth = settings.value("cacheWidth",5).toInt();
    resizeFilter = (Z::ResizeFilter)settings.value("resizeFilter",0).toInt();
    dbUser = settings.value("mysqlUser",QString()).toString();
    dbPass = settings.value("mysqlPassword",QString()).toString();
    dbBase = settings.value("mysqlBase",QString("qmanga")).toString();
    savedAuxOpenDir = settings.value("savedAuxOpenDir",QString()).toString();
    savedIndexOpenDir = settings.value("savedIndexOpenDir",QString()).toString();
    magnifySize = settings.value("magnifySize",150).toInt();
    backgroundColor = QColor(settings.value("backgroundColor","#303030").toString());
    if (!idxFont.fromString(settings.value("idxFont",QString()).toString()))
        idxFont = QApplication::font("QListView");
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

    emit dbSetCredentials(dbBase,dbUser,dbPass);

    if (w!=NULL) {
        if (showMaximized)
            w->showMaximized();

        w->updateBookmarks();
        w->updateViewer();
        w->srcWidget->setEnabled(!dbUser.isEmpty());
        if (w->srcWidget->isEnabled())
            w->srcWidget->updateAlbumsList();
    }
    emit dbCheckBase();
}

void ZGlobal::saveSettings()
{
    QSettings settings("kernel1024", "qmanga");
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
    settings.setValue("idxFont",idxFont.toString());

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

    emit dbCheckEmptyAlbums();
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
    dlg->updateIdxFont(idxFont);
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

    if (dlg->exec()) {
        dbUser=dlg->editMySqlLogin->text();
        dbPass=dlg->editMySqlPassword->text();
        dbBase=dlg->editMySqlBase->text();
        cacheWidth=dlg->spinCacheWidth->value();
        magnifySize=dlg->spinMagnify->value();
        backgroundColor=dlg->getBkColor();
        idxFont=dlg->getIdxFont();
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
        if (w!=NULL) {
            w->updateBookmarks();
            w->updateViewer();
            w->srcWidget->setEnabled(!dbUser.isEmpty());
            if (w->srcWidget->isEnabled())
                w->srcWidget->updateAlbumsList();
        }
        emit dbCheckBase();
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
