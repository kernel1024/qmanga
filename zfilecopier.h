#ifndef ZFILECOPIER_H
#define ZFILECOPIER_H

#include <QObject>
#include <QFileInfo>
#include <QDir>
#include <QAtomicInteger>
#include <QProgressDialog>
#include <QPointer>

class ZFileCopier : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ZFileCopier)
private:
    bool m_abort { false };
    QPointer<QProgressDialog> m_dialog;
    QAtomicInteger<int> m_filesCount;
    QAtomicInteger<int> m_filesCopied;
    QFileInfoList m_srcList;
    QString m_dstDir;
    QStringList m_errors;

    static bool copyFileByInfo(const QFileInfo &src, const QString& dst, QPointer<ZFileCopier> context);
    static int fileInfoGetCount(const QFileInfoList& list, QDir::Filters filter);
    void addFilesCount(int count);

public:
    ZFileCopier(QObject *parent, const QFileInfoList& srcList, QProgressDialog *dialog,
                const QString& dstDir);

Q_SIGNALS:
    void progressSetMaximum(int max);
    void progressSetValue(int val);
    void progressSetLabelText(const QString& str);
    void progressShow();
    void progressClose();
    void errorMsg(const QString& msg);
    void finished();

public Q_SLOTS:
    void start();
    void abort();
};

#endif // ZFILECOPIER_H
