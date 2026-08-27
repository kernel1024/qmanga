#ifndef ZABSTRACTREADER_H
#define ZABSTRACTREADER_H

#include <QImage>
#include <QByteArray>
#include "zglobal.h"

using ZFileEntry = QPair<QString, int>;

class ZAbstractReader : public QObject
{
    Q_OBJECT
protected:
    void postMessage(const QString& msg);
    void performListSort();
    void addSortEntry(const QString& name, int idx);
    int getSortEntryIdx(int num) const;
    void setOpenFileSuccess();
    QString accessPassword();
    void setErrorMessage(const QString& error);

public:
    ZAbstractReader(QObject *parent, const QString &filename, const QString &password);
    ~ZAbstractReader() override;
    bool openFile(const QString &filename);
    int getPageCount();
    bool isOpened() const;
    QString getFileName() const;
    QString getErrorMessage() const;
    QString getSortEntryName(int num) const;

    virtual bool openFile() = 0;
    virtual void closeFile();
    virtual QByteArray loadPage(int num) = 0;
    virtual QImage loadPageImage(int num) = 0;
    virtual QString getMagic() = 0;
    virtual bool isPageDataSupported();
    virtual bool isPasswordProtected();
    virtual QString encryptionMethod();

private:
    bool m_opened { false };
    int m_pageCount { -1 };
    QString m_fileName;
    QString m_accessPassword;
    QString m_errorMessage;
    QVector<ZFileEntry> m_sortList;

    Q_DISABLE_COPY(ZAbstractReader)

};

#endif // ZABSTRACTREADER_H
