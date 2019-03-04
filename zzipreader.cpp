#include <algorithm>
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
    m_sizes.clear();

    QFileInfo fi(fileName);
    if (!fi.isFile() || !fi.isReadable()) {
        return false;
    }

    mainZFile = zip_open(fileName.toUtf8().constData(),ZIP_RDONLY,nullptr);
    if (!mainZFile) {
        return false;
    }

    qint64 count = zip_get_num_entries(mainZFile,0);
    for (qint64 idx = 0; idx < count; idx++) {
        zip_stat_t stat;
        if (zip_stat_index(mainZFile,static_cast<uint>(idx),
                           ZIP_STAT_SIZE | ZIP_STAT_NAME,&stat)<0) {
            qDebug() << "Unable to get file stat for index " << idx;
            continue;
        }
        QString fname = QString::fromUtf8(stat.name);
        if (fname.endsWith('/') || fname.endsWith('\\')) continue;
        QFileInfo fi(fname);
        if (!supportedImg().contains(fi.suffix(),Qt::CaseInsensitive)) continue;
        sortList << ZFileEntry(fname,static_cast<int>(idx));
        m_sizes[static_cast<int>(idx)] = static_cast<int>(stat.size);
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
        zip_close(mainZFile);
    opened = false;
    sortList.clear();
    m_sizes.clear();
}

QByteArray ZZipReader::loadPage(int num)
{
    QByteArray res;
    if (!opened)
        return res;

    int znum = -2;
    if (num>=0 && num<sortList.count())
        znum = sortList.at(num).idx;

    int size = m_sizes.value(znum);
    if (size>(150*1024*1024)) {
        qDebug() << "Image file is too big (over 150Mb). Unable to load.";
        return res;
    }

    res.resize(size);
    zip_file_t* file = zip_fopen_index(mainZFile,static_cast<uint>(znum),0);
    if (file != nullptr) {
        qint64 sz = zip_fread(file,res.data(),static_cast<uint>(size));
        zip_fclose(file);

        if (sz < 0) {
            res.clear();
            qDebug() << "Error while opening compressed image inside archive.";
            return res;
        }
        if (sz<size)
            res.truncate(static_cast<int>(sz));
    }

    return res;
}

QImage ZZipReader::loadPageImage(int num)
{
    const QByteArray buf = loadPage(num);
    if (buf.isEmpty()) return QImage();

    QImage img;
    img.loadFromData(buf);
    return img;
}

QString ZZipReader::getMagic()
{
    return QStringLiteral("ZIP");
}

QString ZZipReader::getInternalPath(int idx)
{
    if (!opened)
        return QString();

    if (idx>=0 && idx<sortList.count())
        return sortList.at(idx).name;

    return QString();
}
