#include "zrarreader.h"

ZRarReader::ZRarReader(QObject *parent, QString filename) :
    ZAbstractReader(parent,filename)
{
    rarExec = QString();
    supportedImg << "jpg" << "jpeg" << "gif" << "png" << "tiff" << "tif" << "bmp";
}

bool ZRarReader::openFile()
{
    if (opened)
        return false;
    sortList.clear();

    QFileInfo fi(fileName);
    if (!fi.isFile() || !fi.isReadable()) {
        return false;
    }

    rarExec = QString();
    if (QProcess::execute("rar",QStringList() << "-inul")<0) {
        if (QProcess::execute("unrar",QStringList() << "-inul")<0)
            return false;
        else
            rarExec = QString("unrar");
    } else
        rarExec = QString("rar");

    int cnt = 0;
    QProcess rar;
    rar.start(rarExec,QStringList() << "vb" << "--" << fileName);
    rar.waitForFinished(60000);
    while (!rar.atEnd()) {
        QString fname = QString::fromUtf8(rar.readLine()).trimmed();
        if (fname.endsWith('/') || fname.endsWith('\\')) continue;
        fi.setFile(fname);
        if (!supportedImg.contains(fi.completeSuffix(),Qt::CaseInsensitive)) continue;
        sortList << ZFileEntry(fname,cnt);
        cnt++;
    }

    qSort(sortList);

    opened = true;
    return true;
}

void ZRarReader::closeFile()
{
    if (!opened)
        return;

    rarExec.clear();
    opened = false;
    sortList.clear();
}

QImage ZRarReader::loadPage(int num)
{
    QImage res;
    if (!opened)
        return res;

    if (num<0 || num>=sortList.count()) return res;

    QString picName = sortList.at(num).name;

    QProcess rar;
    rar.start(rarExec,QStringList() << "p" << "-kb" << "-p-" << "-o-" << "-inul" << "--" << fileName << picName);
    rar.waitForFinished(60000);
    QByteArray buf = rar.readAll();
    if (buf.isEmpty()) return res;
    if (buf.size()>150*1024*1024) {
        qDebug() << "Image file is too big (over 150Mb). Unable to load.";
        return res;
    }

    bool ok = res.loadFromData(buf);
    buf.clear();

    if (!ok) res = QImage();
    return res;
}

QImageHash ZRarReader::loadPages(QIntList nums)
{
    QImageHash hash;
    if (!opened)
        return hash;

    for (int i=0;i<nums.count();i++)
        hash[nums.at(i)] = loadPage(nums.at(i));

    return hash;
}

QString ZRarReader::getMagic()
{
    return QString("RAR");
}
