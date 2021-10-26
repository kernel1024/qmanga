#include <QDebug>
#include "zzipreader.h"

ZZipReader::ZZipReader(QObject *parent, const QString &filename) :
    ZAbstractReader(parent,filename)
{
    m_zFile = nullptr;
}

bool ZZipReader::openFile()
{
    if (isOpened())
        return false;

    m_sizes.clear();

    QString fileName = getFileName();
    QFileInfo fi(fileName);
    if (!fi.isFile() || !fi.isReadable()) {
        return false;
    }

    m_zFile = zip_open(fileName.toUtf8().constData(),ZIP_RDONLY,nullptr);
    if (!m_zFile) {
        return false;
    }

    qint64 count = zip_get_num_entries(m_zFile,0);
    for (qint64 idx = 0; idx < count; idx++) {
        zip_stat_t stat;
        if (zip_stat_index(m_zFile,static_cast<uint>(idx),
                           ZIP_STAT_SIZE | ZIP_STAT_NAME,&stat)<0) {
            qDebug() << "Unable to get file stat for index " << idx;
            continue;
        }
        QString fname = QString::fromUtf8(stat.name);
        if (fname.endsWith('/') || fname.endsWith('\\')) continue;
        QFileInfo fi(fname);
        if (!ZGenericFuncs::supportedImg().contains(fi.suffix(),Qt::CaseInsensitive)) continue;
        addSortEntry(fname,static_cast<int>(idx));
        m_sizes[static_cast<int>(idx)] = static_cast<int>(stat.size);
    }

    performListSort();

    setOpenFileSuccess();
    return true;
}

void ZZipReader::closeFile()
{
    if (!isOpened())
        return;

    if (m_zFile!=nullptr)
        zip_close(m_zFile);
    m_sizes.clear();
    ZAbstractReader::closeFile();
}

QByteArray ZZipReader::loadPage(int num)
{
    QByteArray res;
    if (!isOpened())
        return res;

    int znum = getSortEntryIdx(num);
    if (znum<0) {
        qCritical() << "Incorrect page number";
        return res;
    }

    int size = m_sizes.value(znum);
    if (size>ZDefaults::maxImageFileSize) {
        qDebug() << "Image file is too big (over 150Mb). Unable to load.";
        return res;
    }

    res.resize(size);
    zip_file_t* file = zip_fopen_index(m_zFile,static_cast<uint>(znum),0);
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
    return QSL("ZIP");
}
