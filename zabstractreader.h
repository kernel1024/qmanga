#ifndef ZABSTRACTREADER_H
#define ZABSTRACTREADER_H

#include <QImage>
#include <QByteArray>
#include "zglobal.h"

class ZAbstractReader : public QObject
{
    Q_OBJECT
protected:
    void postMessage(const QString& msg);
    void performListSort();
    void addSortEntry(const ZFileEntry& entry);
    int getSortEntryIdx(int num) const;
    QString getSortEntryName(int num) const;
    void setOpenFileSuccess();

public:
    explicit ZAbstractReader(QObject *parent, const QString &filename);
    ~ZAbstractReader() override;
    bool openFile(const QString &filename);
    int getPageCount();
    bool isOpened() const;
    QString getFileName() const;
    QString getInternalPath(int idx);

    virtual bool openFile() = 0;
    virtual void closeFile();
    virtual QByteArray loadPage(int num) = 0;
    virtual QImage loadPageImage(int num) = 0;
    virtual QString getMagic() = 0;

private:
    bool m_opened { false };
    int m_pageCount { -1 };
    QString m_fileName;
    QList<ZFileEntry> m_sortList;

    Q_DISABLE_COPY(ZAbstractReader)

};

#endif // ZABSTRACTREADER_H
