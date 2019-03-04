#include <algorithm>
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
    static const QStringList rarTestParam { "-inul" };
    static const QStringList rarListParam { "vb", "--" };

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
        if (QProcess::execute(QStringLiteral("rar"),rarTestParam)<0) {
            if (QProcess::execute(QStringLiteral("unrar"),rarTestParam)<0)
                return false;

            rarExec = QStringLiteral("unrar");
        } else
            rarExec = QStringLiteral("rar");
#else
        rarExec = QStringLiteral("rar.exe");
#endif
    }

    int cnt = 0;
    QProcess rar;
    QStringList rarListParamList = rarListParam;
    rarListParamList << fileName;
    rar.start(rarExec,rarListParamList);
    rar.waitForFinished(60000);
    while (!rar.atEnd()) {
        QString fname = QString::fromUtf8(rar.readLine()).trimmed();
        if (fname.endsWith('/') || fname.endsWith('\\')) continue;
        fi.setFile(fname);
        if (!supportedImg().contains(fi.suffix(),Qt::CaseInsensitive)) continue;
        sortList << ZFileEntry(fname,cnt);
        cnt++;
    }

    std::sort(sortList.begin(),sortList.end());

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
    static const QStringList rarExtractParam { "p", "-kb", "-p-", "-o-", "-inul", "--" };

    QByteArray res;
    res.clear();
    if (!opened)
        return res;

    if (num<0 || num>=sortList.count()) return res;

    QString picName = sortList.at(num).name;

    QProcess rar;
    QStringList rarExtractParamList = rarExtractParam;
    rarExtractParamList << fileName << picName;
    rar.start(rarExec,rarExtractParamList);
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

QImage ZRarReader::loadPageImage(int num)
{
    const QByteArray buf = loadPage(num);
    if (buf.isEmpty()) return QImage();

    QImage img;
    img.loadFromData(buf);
    return img;
}

QString ZRarReader::getMagic()
{
    return QStringLiteral("RAR");
}

QString ZRarReader::getInternalPath(int idx)
{
    if (!opened)
        return QString();

    if (idx>=0 && idx<sortList.count())
        return sortList.at(idx).name;

    return QString();
}
