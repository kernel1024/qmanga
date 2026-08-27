#include <QProcess>
#include <QDebug>
#include "zrarreader.h"

ZRarReader::ZRarReader(QObject *parent, const QString &filename, const QString &password) :
    ZAbstractReader(parent,filename,password)
{
    m_rarExec = QString();
}

bool ZRarReader::openFile()
{
    static const QStringList rarTestParam { QSL("-inul") };
    static const QStringList rarListParam { QSL("vb"), QSL("--") };
    static const QStringList rarPasswdCheckParam { QSL("vt"), QSL("-p-"), QSL("--") };

    if (isOpened() || !zF->global())
        return false;

    QFileInfo fi(getFileName());
    if (!fi.isFile() || !fi.isReadable()) {
        return false;
    }

    m_rarExec = zF->global()->getRarCmd();
    if (m_rarExec.isEmpty()) {
#ifndef Q_OS_WIN
        if (QProcess::execute(QSL("rar"),rarTestParam)<0) {
            if (QProcess::execute(QSL("unrar"),rarTestParam)<0)
                return false;

            m_rarExec = QSL("unrar");
        } else {
            m_rarExec = QSL("rar");
        }
#else
        m_rarExec = QSL("rar.exe");
#endif
    }

    m_passwordProtected = false;
    int cnt = 0;
    QProcess rar;
    QStringList rarListParamList = rarListParam;
    rarListParamList << getFileName();
    rar.start(m_rarExec,rarListParamList);
    rar.waitForFinished(ZDefaults::oneMinuteMS);
    while (!rar.atEnd()) {
        QString fname = QString::fromUtf8(rar.readLine()).trimmed();
        if (fname.endsWith(u'/') || fname.endsWith(u'\\')) continue;
        fi.setFile(fname);
        if (!ZGenericFuncs::supportedImg().contains(fi.suffix(),Qt::CaseInsensitive)) continue;
        addSortEntry(fname,cnt);
        cnt++;
    }

    // Check for the password protection
    if (cnt>0) {
        QProcess rar2;
        QStringList rarListParamList2 = rarPasswdCheckParam;
        rarListParamList2 << getFileName();
        rar2.start(m_rarExec,rarListParamList2);
        rar2.waitForFinished(ZDefaults::oneMinuteMS);
        while (!rar2.atEnd()) {
            QString fname = QString::fromUtf8(rar2.readLine()).trimmed();
            static const QRegularExpression re(QSL("Flags:\\s+encrypted"));
            if (fname.contains(re))
                m_passwordProtected = true;
        }
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
    QString rarPasswd = QSL("-");
    if (isPasswordProtected())
        rarPasswd = accessPassword();

    const QStringList rarExtractParam { QSL("p"), QSL("-kb"), QSL("-p%1").arg(rarPasswd),
                                      QSL("-o-"), QSL("-inul"), QSL("--") };
    QByteArray res;
    if (!isOpened())
        return res;

    QString picName = getSortEntryName(num);
    if (picName.isEmpty()) {
        setErrorMessage(QSL("Incorrect page number"));
        return res;
    }

    QProcess rar;
    QStringList rarExtractParamList = rarExtractParam;
    rarExtractParamList << getFileName() << picName;
    rar.start(m_rarExec,rarExtractParamList);
    rar.waitForFinished(ZDefaults::oneMinuteMS);

    if (rar.exitStatus() == QProcess::ExitStatus::NormalExit) {
        switch (rar.exitCode()) {
            case 2: setErrorMessage(QSL("A fatal error occurred.")); return res;
            case 3: setErrorMessage(QSL("Invalid checksum. Data is damaged.")); return res;
            case 4: setErrorMessage(QSL("Attempt to modify an archive locked by 'k' command.")); return res;
            case 5: setErrorMessage(QSL("Write error.")); return res;
            case 6: setErrorMessage(QSL("File open error.")); return res;
            case 7: setErrorMessage(QSL("Wrong command line option.")); return res;
            case 8: setErrorMessage(QSL("Not enough memory.")); return res;
            case 9: setErrorMessage(QSL("File create error.")); return res;
            case 10: setErrorMessage(QSL("No files matching the specified mask and options were found.")); return res;
            case 11: setErrorMessage(QSL("Wrong password.")); return res;
            case 255: setErrorMessage(QSL("User stopped the process.")); return res;
        }
    } else {
        setErrorMessage(QSL("RAR binary crashed."));
        return res;
    }

    if (rar.bytesAvailable()>ZDefaults::maxImageFileSize) {
        setErrorMessage(QSL("Image file is too big. Unable to load."));
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

bool ZRarReader::isPasswordProtected()
{
    return m_passwordProtected;
}

QString ZRarReader::encryptionMethod()
{
    if (isPasswordProtected())
        return QSL("RAR");

    return QString();
}
