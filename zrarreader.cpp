#include <QProcess>
#include <QDebug>
#include "zrarreader.h"

ZRarReader::ZRarReader(QObject *parent, const QString &filename) :
    ZAbstractReader(parent,filename)
{
    m_rarExec = QString();
}

bool ZRarReader::openFile()
{
    static const QStringList rarTestParam { "-inul" };
    static const QStringList rarListParam { "vb", "--" };

    if (isOpened() || !zg)
        return false;

    QFileInfo fi(getFileName());
    if (!fi.isFile() || !fi.isReadable()) {
        return false;
    }

    m_rarExec = zg->rarCmd;
    if (m_rarExec.isEmpty()) {
#ifndef Q_OS_WIN
        if (QProcess::execute(QSL("rar"),rarTestParam)<0) {
            if (QProcess::execute(QSL("unrar"),rarTestParam)<0)
                return false;

            m_rarExec = QSL("unrar");
        } else
            m_rarExec = QSL("rar");
#else
        rarExec = QStringLiteral("rar.exe");
#endif
    }

    int cnt = 0;
    QProcess rar;
    QStringList rarListParamList = rarListParam;
    rarListParamList << getFileName();
    rar.start(m_rarExec,rarListParamList);
    rar.waitForFinished(ZDefaults::oneMinuteMS);
    while (!rar.atEnd()) {
        QString fname = QString::fromUtf8(rar.readLine()).trimmed();
        if (fname.endsWith('/') || fname.endsWith('\\')) continue;
        fi.setFile(fname);
        if (!supportedImg().contains(fi.suffix(),Qt::CaseInsensitive)) continue;
        addSortEntry(ZFileEntry(fname,cnt));
        cnt++;
    }

    performListSort();
    setOpenFileSuccess();

    return true;
}

void ZRarReader::closeFile()
{
    if (!isOpened())
        return;

    m_rarExec.clear();
    ZAbstractReader::closeFile();
}

QByteArray ZRarReader::loadPage(int num)
{
    static const QStringList rarExtractParam { "p", "-kb", "-p-", "-o-", "-inul", "--" };

    QByteArray res;
    if (!isOpened())
        return res;

    QString picName = getSortEntryName(num);
    if (picName.isEmpty()) {
        qCritical() << "Incorrect page number";
        return res;
    }

    QProcess rar;
    QStringList rarExtractParamList = rarExtractParam;
    rarExtractParamList << getFileName() << picName;
    rar.start(m_rarExec,rarExtractParamList);
    rar.waitForFinished(ZDefaults::oneMinuteMS);
    if (rar.bytesAvailable()>ZDefaults::maxImageFileSize) {
        qDebug() << "Image file is too big. Unable to load.";
        return res;
    }
    res = rar.readAll();
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
    return QSL("RAR");
}
