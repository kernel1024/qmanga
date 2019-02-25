#ifndef ZABSTRACTREADER_H
#define ZABSTRACTREADER_H

#include <QImage>
#include <QByteArray>
#include "zglobal.h"

class ZAbstractReader : public QObject
{
    Q_OBJECT
protected:
    bool opened;
    int pageCount;
    QString fileName;
    QList<ZFileEntry> sortList;

public:
    explicit ZAbstractReader(QObject *parent, const QString &filename);
    ~ZAbstractReader();
    bool openFile(const QString &filename);
    int getPageCount();

    virtual bool openFile() = 0;
    virtual void closeFile();
    virtual QByteArray loadPage(int num) = 0;
    virtual QImage loadPageImage(int num) = 0;
    virtual QString getMagic() = 0;
    virtual QString getInternalPath(int idx);
    
signals:
    void auxMessage(const QString& msg);
        
};

#endif // ZABSTRACTREADER_H
