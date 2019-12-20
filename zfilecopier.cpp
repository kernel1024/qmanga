#include <QByteArray>
#include <QApplication>
#include <QFile>
#include <QDir>
#include <QDebug>
#include "zfilecopier.h"
#include "global.h"

ZFileCopier::ZFileCopier(const QFileInfoList& srcList, QProgressDialog *dialog,
                         const QString &dstDir, QObject *parent) : QObject(parent)
{
    m_dialog = dialog;
    m_srcList = srcList;
    m_dstDir = dstDir;
    m_abort = false;

    m_dialog->setAttribute(Qt::WA_DeleteOnClose);
    m_dialog->setAutoClose(false);
    m_dialog->setMinimum(0);
    m_dialog->setValue(0);
    m_dialog->setLabelText(QString());
    m_dialog->show();

    connect(this,&ZFileCopier::progressSetValue,m_dialog,&QProgressDialog::setValue,Qt::QueuedConnection);
    connect(this,&ZFileCopier::progressSetMaximum,m_dialog,&QProgressDialog::setMaximum,Qt::QueuedConnection);
    connect(this,&ZFileCopier::progressSetLabelText,m_dialog,&QProgressDialog::setLabelText,Qt::QueuedConnection);
    connect(this,&ZFileCopier::progressShow,m_dialog,&QProgressDialog::show,Qt::QueuedConnection);
    connect(this,&ZFileCopier::progressClose,m_dialog,&QProgressDialog::close,Qt::QueuedConnection);
}

void ZFileCopier::start()
{
    QStringList errors;
    m_abort = false;
    int cnt = 0;

    Q_EMIT progressShow();
    Q_EMIT progressSetMaximum(m_srcList.count());
    QApplication::processEvents();
    for (const QFileInfo& fi : qAsConst(m_srcList)) {
        Q_EMIT progressSetLabelText(tr("Copying %1...").arg(fi.fileName()));
        Q_EMIT progressSetValue(0);
        QApplication::processEvents();
        bool res;
        if (fi.isDir()) {
            res = copyDir(fi);
        } else {
            res = copyFile(fi);
        }

        if (!res) {
            errors << fi.fileName();
        } else {
            cnt++;
        }

        if (m_abort)
            break;
    }

    if (m_abort) {
        Q_EMIT errorMsg(tr("Copying aborted, %1 files was copied.").arg(cnt));
    } else if (!errors.empty()) {
        Q_EMIT errorMsg(tr("Failed to copy %1 entries:\n%2").arg(errors.count()).arg(errors.join('\n')));
    }

    Q_EMIT progressClose();
    QApplication::processEvents();
    Q_EMIT finished();
}

void ZFileCopier::abort()
{
    m_abort = true;
}

bool ZFileCopier::copyDir(const QFileInfo &src)
{
    // copy without subdirs - for image catalog manga entries
    QDir ddir(m_dstDir);
    if (!ddir.mkdir(src.fileName()))
        return false;

    ddir.setPath(ddir.absoluteFilePath(src.fileName()));
    if (!ddir.exists())
        return false;

    QDir sdir(src.filePath());
    if (!sdir.isReadable())
        return false;

    const QFileInfoList fl = filterSupportedImgFiles(
                                 sdir.entryInfoList(QStringList(QSL("*")),
                                                    QDir::Readable | QDir::Files));

    Q_EMIT progressSetMaximum(fl.count());
    QApplication::processEvents();
    for (int i=0;i<fl.count();i++) {
        if (!copyFile(fl.at(i),ddir.absolutePath()))
            return false;

        Q_EMIT progressSetValue(i);
        QApplication::processEvents();
    }

    return true;
}

bool ZFileCopier::copyFile(const QFileInfo &src, const QString &dst)
{
    QFile fsrc(src.filePath());
    if (!fsrc.open(QIODevice::ReadOnly)) return false;

    QDir ddir(m_dstDir);
    bool ui = true;
    if (!dst.isEmpty()) {
        ddir.setPath(dst);
        ui = false;
    }

    if (!ddir.exists()) {
        fsrc.close();
        return false;
    }
    QString dfname =  ddir.absoluteFilePath(src.fileName());
    QFile fdst(dfname);
    if (!fdst.open(QIODevice::WriteOnly)) {
        fsrc.close();
        return false;
    }

    Q_EMIT progressSetMaximum(100);
    QApplication::processEvents();
    qint64 sz = fsrc.size();
    qint64 pos = 0;
    QByteArray buf;
    qint64 written = 0;
    do {
        buf = fsrc.read(ZDefaults::copyBlockSize);
        written = fdst.write(buf);
        if (written>=0 && ui) {
            pos += written;
            Q_EMIT progressSetValue(static_cast<int>(100*pos/sz));
            QApplication::processEvents();
        }
    } while(!buf.isEmpty() && written>0 && !m_abort);

    fsrc.close();
    fdst.close();
    Q_EMIT progressSetValue(100);
    QApplication::processEvents();

    if (written<0 || m_abort) { // write error
        fdst.remove();
        return false;
    }

    return true;
}
