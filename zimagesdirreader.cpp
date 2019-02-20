#include <QDebug>
#include "zimagesdirreader.h"

ZImagesDirReader::ZImagesDirReader(QObject *parent, const QString &filename)
    : ZAbstractReader(parent,filename)
{

}

bool ZImagesDirReader::openFile()
{
    if (opened)
        return false;

    sortList.clear();

    QDir d(fileName);
    if (!d.isReadable()) return false;
    QFileInfoList fl = d.entryInfoList(QStringList("*"), QDir::Readable | QDir::Files);
    filterSupportedImgFiles(fl);
    for (int i=0;i<fl.count();i++)
        sortList << ZFileEntry(fl.at(i).absoluteFilePath(),i);

    qSort(sortList);

    opened = true;
    return true;
}

void ZImagesDirReader::closeFile()
{
    if (!opened)
        return;

    opened = false;
    sortList.clear();
}

QByteArray ZImagesDirReader::loadPage(int num)
{
    QByteArray res;
    res.clear();
    if (!opened)
        return res;

    int idx = 0;
    int znum = -2;
    if (num>=0 && num<sortList.count())
        znum = sortList.at(num).idx;

    QDir d(fileName);
    if (!d.isReadable()) return res;
    QFileInfoList fl = d.entryInfoList(QStringList("*"), QDir::Readable | QDir::Files);
    filterSupportedImgFiles(fl);
    for (const auto &fi : fl) {
        if (idx==znum) {
            if (fi.size()>(150*1024*1024)) {
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

QString ZImagesDirReader::getMagic()
{
    return QString("DYN");
}

QString ZImagesDirReader::getInternalPath(int idx)
{
    if (!opened)
        return QString();

    if (idx>=0 && idx<sortList.count())
        return sortList.at(idx).name;

    return QString();
}
