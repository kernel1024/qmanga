#include "zglobal.h"
#include "mainwindow.h"
#include "settingsdialog.h"
#include <ImageMagick/Magick++.h>

using namespace Magick;

ZGlobal* zGlobal = NULL;

ZGlobal::ZGlobal(QObject *parent) :
    QObject(parent)
{
    cacheWidth = 5;
    bookmarks.clear();
}

void ZGlobal::loadSettings()
{
    MainWindow* w = qobject_cast<MainWindow *>(parent());

    QSettings settings("kilobax", "qmanga");
    settings.beginGroup("MainWindow");
    cacheWidth = settings.value("cacheWidth",5).toInt();
    resizeFilter = (ZResizeFilter)settings.value("resizeFilter",0).toInt();
    mysqlUser = settings.value("mysqlUser",QString()).toString();
    mysqlPassword = settings.value("mysqlPassword",QString()).toString();
    savedAuxOpenDir = settings.value("savedAuxOpenDir",QString()).toString();
    magnifySize = settings.value("magnifySize",150).toInt();

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

    settings.endGroup();

    if (w!=NULL) {
        if (showMaximized)
            w->showMaximized();

        w->updateBookmarks();
    }
}

void ZGlobal::saveSettings()
{
    QSettings settings("kilobax", "qmanga");
    settings.beginGroup("MainWindow");
    settings.setValue("cacheWidth",cacheWidth);
    settings.setValue("resizeFilter",(int)resizeFilter);
    settings.setValue("mysqlUser",mysqlUser);
    settings.setValue("mysqlPassword",mysqlPassword);
    settings.setValue("savedAuxOpenDir",savedAuxOpenDir);
    settings.setValue("magnifySize",magnifySize);

    MainWindow* w = qobject_cast<MainWindow *>(parent());
    if (w!=NULL) {
        settings.setValue("maximized",w->isMaximized());
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

void ZGlobal::settingsDlg()
{
    MainWindow* w = qobject_cast<MainWindow *>(parent());
    SettingsDialog* dlg = new SettingsDialog(w);

    dlg->editMySqlLogin->setText(mysqlUser);
    dlg->editMySqlPassword->setText(mysqlPassword);
    dlg->spinCacheWidth->setValue(cacheWidth);
    dlg->spinMagnify->setValue(magnifySize);
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
        mysqlUser=dlg->editMySqlLogin->text();
        mysqlPassword=dlg->editMySqlPassword->text();
        cacheWidth=dlg->spinCacheWidth->value();
        magnifySize=dlg->spinMagnify->value();
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
        if (w!=NULL)
            w->updateBookmarks();
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
    UNUSED(selectedFilter);
    UNUSED(options);
    return KFileDialog::getOpenFileName(KUrl(dir),getKDEFilters(filter),parent,caption);
#else
    return QFileDialog::getOpenFileName(parent,caption,dir,filter,selectedFilter,options);
#endif
}

QStringList ZGlobal::getOpenFileNamesD ( QWidget * parent, const QString & caption, const QString & dir, const QString & filter, QString * selectedFilter, QFileDialog::Options options )
{
#ifdef QB_KDEDIALOGS
    UNUSED(selectedFilter);
    UNUSED(options);
    return KFileDialog::getOpenFileNames(KUrl(dir),getKDEFilters(filter),parent,caption);
#else
    return QFileDialog::getOpenFileNames(parent,caption,dir,filter,selectedFilter,options);
#endif
}

QString ZGlobal::getSaveFileNameD ( QWidget * parent, const QString & caption, const QString & dir, const QString & filter, QString * selectedFilter, QFileDialog::Options options )
{
#ifdef QB_KDEDIALOGS
    UNUSED(selectedFilter);
    UNUSED(options);
    return KFileDialog::getSaveFileName(KUrl(dir),getKDEFilters(filter),parent,caption);
#else
    return QFileDialog::getSaveFileName(parent,caption,dir,filter,selectedFilter,options);
#endif
}

QString	ZGlobal::getExistingDirectoryD ( QWidget * parent, const QString & caption, const QString & dir, QFileDialog::Options options )
{
#ifdef QB_KDEDIALOGS
    UNUSED(options);
    return KFileDialog::getExistingDirectory(KUrl(dir),parent,caption);
#else
    return QFileDialog::getExistingDirectory(parent,caption,dir,options);
#endif
}
