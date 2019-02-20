#include <QProcess>
#include <QDebug>
#include "zrarreader.h"

ZRarReader::ZRarReader(QObject *parent, const QString &filename) :
    ZAbstractReader(parent,filename)
{
    rarExec = QString();
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

    rarExec = zg->rarCmd;
    if (rarExec.isEmpty()) {
#ifndef Q_OS_WIN
        if (QProcess::execute("rar",QStringList() << "-inul")<0) {
            if (QProcess::execute("unrar",QStringList() << "-inul")<0)
                return false;

            rarExec = QString("unrar");
        } else
            rarExec = QString("rar");
#else
        rarExec = QString("rar.exe");
#endif
    }

    int cnt = 0;
    QProcess rar;
    rar.start(rarExec,QStringList() << "vb" << "--" << fileName);
    rar.waitForFinished(60000);
    while (!rar.atEnd()) {
        QString fname = QString::fromUtf8(rar.readLine()).trimmed();
        if (fname.endsWith('/') || fname.endsWith('\\')) continue;
        fi.setFile(fname);
        if (!supportedImg().contains(fi.suffix(),Qt::CaseInsensitive)) continue;
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

QByteArray ZRarReader::loadPage(int num)
{
    QByteArray res;
    res.clear();
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

    res = buf;
    buf.clear();

    return res;
}

QString ZRarReader::getMagic()
{
    return QString("RAR");
}

QString ZRarReader::getInternalPath(int idx)
{
    if (!opened)
        return QString();

    if (idx>=0 && idx<sortList.count())
        return sortList.at(idx).name;

    return QString();
}
