#ifndef ZFILECOPIER_H
#define ZFILECOPIER_H

#include <QObject>
#include <QFileInfo>
#include <QProgressDialog>

class ZFileCopier : public QObject
{
    Q_OBJECT
private:
    QFileInfoList m_srcList;
    QProgressDialog *m_dialog;
    QString m_dstDir;
    bool m_abort;

    bool copyDir(const QFileInfo &src);
    bool copyFile(const QFileInfo &src, const QString& dst = QString());

public:
    explicit ZFileCopier(const QFileInfoList& srcList, QProgressDialog *dialog,
                         const QString& dstDir, QObject *parent = 0);

signals:
    void progressSetMaximum(int max);
    void progressSetValue(int val);
    void progressSetLabelText(const QString& str);
    void progressShow();
    void progressClose();
    void errorMsg(const QString& msg);
    void finished();

public slots:
    void start();
    void abort();
};

#endif // ZFILECOPIER_H
