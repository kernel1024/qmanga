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

public:
    ZAbstractReader(QObject *parent, const QString &filename);
    ~ZAbstractReader() override;
    bool openFile(const QString &filename);
    int getPageCount();
    bool isOpened() const;
    QString getFileName() const;
    QString getSortEntryName(int num) const;

    virtual bool openFile() = 0;
    virtual void closeFile();
    virtual QByteArray loadPage(int num) = 0;
    virtual QImage loadPageImage(int num) = 0;
    virtual QString getMagic() = 0;
    virtual bool isPageDataSupported();

private:
    bool m_opened { false };
    int m_pageCount { -1 };
    QString m_fileName;
    QVector<ZFileEntry> m_sortList;

    Q_DISABLE_COPY(ZAbstractReader)

};

#endif // ZABSTRACTREADER_H
