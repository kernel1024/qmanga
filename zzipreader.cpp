#include <algorithm>
#include <zzip/zzip.h>
#include <QDebug>
#include "zzipreader.h"

ZZipReader::ZZipReader(QObject *parent, const QString &filename) :
    ZAbstractReader(parent,filename)
{
    mainZFile = nullptr;
}

bool ZZipReader::openFile()
{
    if (opened)
        return false;

    sortList.clear();
    QFileInfo fi(fileName);
    if (!fi.isFile() || !fi.isReadable()) {
        return false;
    }

    mainZFile = zzip_opendir(fileName.toUtf8().data());
    if (!mainZFile) {
        return false;
    }

    int cnt = 0;
    ZZIP_DIRENT* d;
    while ((d=zzip_readdir(mainZFile))) {
        QString fname(d->d_name);
        if (fname.endsWith('/') || fname.endsWith('\\')) continue;
        QFileInfo fi(fname);
        if (!supportedImg().contains(fi.suffix(),Qt::CaseInsensitive)) continue;
        sortList << ZFileEntry(fname,cnt);
        cnt++;
    }

    std::sort(sortList.begin(),sortList.end());

    opened = true;
    return true;
}

void ZZipReader::closeFile()
{
    if (!opened)
        return;

    if (mainZFile!=nullptr)
        zzip_closedir(mainZFile);
    opened = false;
    sortList.clear();
}

QByteArray ZZipReader::loadPage(int num)
{
    QByteArray res;
    if (!opened)
        return res;

    int idx = 0;
    int znum = -2;
    if (num>=0 && num<sortList.count())
        znum = sortList.at(num).idx;

    ZZIP_DIRENT* d;
    zzip_rewinddir(mainZFile);
    while ((d=zzip_readdir(mainZFile))) {
        QString fname(d->d_name);
        if (fname.endsWith('/') || fname.endsWith('\\')) continue;
        QFileInfo fi(fname);
        if (!supportedImg().contains(fi.suffix(),Qt::CaseInsensitive)) continue;

        if (idx==znum) {
            if (d->st_size>(150*1024*1024)) {
                qDebug() << "Image file is too big (over 150Mb). Unable to load.";
                return res;
            }
            ZZIP_FILE* zf = zzip_file_open(mainZFile, d->d_name, O_RDONLY);
            if (!zf) {
                qDebug() << "Error while opening compressed image inside archive.";
                return res;
            }

            res.resize(d->st_size);
            zzip_ssize_t n = zzip_read(zf, res.data(), static_cast<zzip_size_t>(d->st_size));
            if (n < d->st_size)
                res.truncate(static_cast<int>(n));

            zzip_file_close(zf);
            return res;
        }
        idx++;
    }
    return res;
}

QString ZZipReader::getMagic()
{
    return QString("ZIP");
}

QString ZZipReader::getInternalPath(int idx)
{
    if (!opened)
        return QString();

    if (idx>=0 && idx<sortList.count())
        return sortList.at(idx).name;

    return QString();
}
