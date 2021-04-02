#include <algorithm>
#include <QByteArray>
#include <QApplication>
#include <QFile>
#include <QDir>
#include <QDebug>
#include "zfilecopier.h"
#include "global.h"

ZFileCopier::ZFileCopier(QObject *parent, const QFileInfoList& srcList, QProgressDialog *dialog,
                         const QString &dstDir)
    : QObject(parent),
      m_dialog(dialog),
      m_srcList(srcList),
      m_dstDir(dstDir)
{

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
    m_filesCount = 0;
    m_filesCopied = 0;
    m_errors.clear();
    m_errors.reserve(m_srcList.count());
    m_abort = false;

    Q_EMIT progressShow();
    addFilesCount(ZFileCopier::fileInfoGetCount(m_srcList,QDir::Files));

    QThread *th = QThread::create([this](){
        for (const QFileInfo& fi : qAsConst(m_srcList)) {
            if (!ZFileCopier::copyFileByInfo(fi,m_dstDir,this)) {
                m_errors.append(fi.fileName());
            }

            if (m_abort)
                break;
        }

        QMetaObject::invokeMethod(this,[this](){
            Q_EMIT progressClose();

            if (m_abort) {
                Q_EMIT errorMsg(tr("Copying aborted, %1 files was copied.").arg(m_filesCopied));
            } else if (!m_errors.empty()) {
                Q_EMIT errorMsg(tr("Failed to copy %1 entries:\n%2")
                                .arg(m_errors.count())
                                .arg(m_errors.join('\n')));
            }
            Q_EMIT finished();
        },Qt::QueuedConnection);
    });
    connect(th,&QThread::finished,th,&QThread::deleteLater);
    th->setObjectName(QSL("FCOPY_worker"));
    th->start();
}

void ZFileCopier::abort()
{
    m_abort = true;
}

bool ZFileCopier::copyFileByInfo(const QFileInfo &src, const QString &dst, QPointer<ZFileCopier> context)
{
    if (context.isNull())
        return false;

    QDir destDir(dst);
    if (!destDir.exists())
        return false;

    if (src.isDir()) {
        if (!destDir.exists(src.fileName())) {
            if (!destDir.mkdir(src.fileName()))
                return false;
        }

        QDir sourceDir(src.filePath());
        if (!sourceDir.isReadable())
            return false;

        const QFileInfoList fl = sourceDir.entryInfoList({ QSL("*") },
                                                         QDir::Readable | QDir::Files | QDir::AllDirs |
                                                         QDir::NoDotAndDotDot | QDir::NoSymLinks);
        if (context) {
            QMetaObject::invokeMethod(context,[context,fl](){
                context->addFilesCount(ZFileCopier::fileInfoGetCount(fl,QDir::Files));
            },Qt::QueuedConnection);
        }

        for (const auto &fi : fl) {
            if (!copyFileByInfo(fi,destDir.filePath(src.fileName()),context) && (!context.isNull()))
                context->m_errors.append(fi.fileName());
        }

        return true;
    }

    if (src.isFile()) {
        QFile fsrc(src.filePath());
        if (!fsrc.open(QIODevice::ReadOnly))
            return false;

        QFile fdst(destDir.filePath(src.fileName()));
        if (!fdst.open(QIODevice::WriteOnly)) {
            fsrc.close();
            return false;
        }

        const QString onlyName = src.fileName();
        if (context) {
            QMetaObject::invokeMethod(context,[context,onlyName](){
                Q_EMIT context->progressSetLabelText(tr("Copying %1...").arg(onlyName));
            },Qt::QueuedConnection);
        }

        qint64 pos = 0;
        QByteArray buf;
        qint64 written = 0;
        do {
            buf = fsrc.read(ZDefaults::copyBlockSize);
            written = fdst.write(buf);
            if (written>=0) {
                pos += written;
            }
        } while (!buf.isEmpty() && written>0 && !context.isNull() && !context->m_abort);

        fsrc.close();
        fdst.close();

        if (written<0 || context.isNull() || context->m_abort) { // write error
            fdst.remove();
            return false;
        }

        if (context) {
            QMetaObject::invokeMethod(context,[context](){
                context->m_filesCopied++;
                Q_EMIT context->progressSetValue(context->m_filesCopied);
            },Qt::QueuedConnection);
        }
    }

    // Links and other stuff? Just skip them.
    return true;
}

int ZFileCopier::fileInfoGetCount(const QFileInfoList &list, QDir::Filters filter)
{
    int res = 0;
    for (const auto &fi : list) {
        if (((filter & QDir::AllDirs) > 0) && fi.isDir()) {
            res++;
            continue;
        }

        if (((filter & QDir::NoSymLinks) > 0) && fi.isSymbolicLink()) continue;
        if (((filter & QDir::NoDot) > 0) && (fi.fileName() == QSL("."))) continue;
        if (((filter & QDir::NoDotDot) > 0) && (fi.fileName() == QSL(".."))) continue;

        if ((filter & QDir::PermissionMask) > 0) {
            if (((filter & QDir::Executable) > 0) && !fi.isExecutable()) continue;
            if (((filter & QDir::Writable) > 0) && !fi.isReadable()) continue;
            if (((filter & QDir::Readable) > 0) && !fi.isWritable()) continue;
        }

        if (((filter & QDir::Files) > 0) && fi.isFile()) {
            res++;
            continue;
        }
        if (((filter & QDir::Dirs) > 0) && fi.isDir()) {
            res++;
            continue;
        }
    }
    return res;
}

void ZFileCopier::addFilesCount(int count)
{
    m_filesCount += count;
    Q_EMIT progressSetMaximum(m_filesCount);
}
