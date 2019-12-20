#include <algorithm>
#include <QDebug>
#include "zimagesdirreader.h"

ZImagesDirReader::ZImagesDirReader(QObject *parent, const QString &filename)
    : ZAbstractReader(parent,filename)
{

}

bool ZImagesDirReader::openFile()
{
    if (isOpened())
        return false;

    QDir d(getFileName());
    if (!d.isReadable()) return false;
    const QFileInfoList fl = filterSupportedImgFiles(
                                 d.entryInfoList(
                                     QStringList(QSL("*")),
                                     QDir::Readable | QDir::Files));
    for (int i=0;i<fl.count();i++)
        addSortEntry(ZFileEntry(fl.at(i).absoluteFilePath(),i));

    performListSort();
    setOpenFileSuccess();
    return true;
}

QByteArray ZImagesDirReader::loadPage(int num)
{
    QByteArray res;
    if (!isOpened())
        return res;

    int znum = getSortEntryIdx(num);
    if (znum<0) {
        qCritical() << "Incorrect page number";
        return res;
    }

    QDir d(getFileName());
    if (!d.isReadable()) return res;
    const QFileInfoList fl = filterSupportedImgFiles(
                                 d.entryInfoList(
                                     QStringList(QSL("*")),
                                     QDir::Readable | QDir::Files));
    int idx = 0;
    for (const auto &fi : fl) {
        if (idx==znum) {
            if (fi.size()>ZDefaults::maxImageFileSize) {
                qDebug() << "Image file is too big (over 150Mb). Unable to load.";
                return res;
            }
            QFile f(fi.absoluteFilePath());
            if (!f.open(QIODevice::ReadOnly)) {
                qDebug() << "Error while opening image file." << fi.absoluteFilePath();
                return res;
            }
            res=f.readAll();
            f.close();
            return res;
        }
        idx++;
    }
    return res;
}

QImage ZImagesDirReader::loadPageImage(int num)
{
    const QByteArray buf = loadPage(num);
    if (buf.isEmpty()) return QImage();

    QImage img;
    img.loadFromData(buf);
    return img;
}

QString ZImagesDirReader::getMagic()
{
    return QSL("DYN");
}
