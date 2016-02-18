#ifndef ZABSTRACTREADER_H
#define ZABSTRACTREADER_H

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
    explicit ZAbstractReader(QObject *parent, QString filename);
    ~ZAbstractReader();
    bool openFile(QString filename);
    bool isOpened();
    int getPageCount();

    virtual bool openFile() = 0;
    virtual void closeFile();
    virtual QByteArray loadPage(int num) = 0;
    virtual QByteHash loadPages(QIntList nums);
    virtual QString getMagic() = 0;
    virtual QString getInternalPath(int idx);
    
signals:
    void auxMessage(const QString& msg);
    
public slots:
    
};

#endif // ZABSTRACTREADER_H
