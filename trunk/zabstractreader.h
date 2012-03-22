#ifndef ZABSTRACTREADER_H
#define ZABSTRACTREADER_H

#include <QtCore>
#include <QtGui>
#include "zglobal.h"

class ZAbstractReader : public QObject
{
    Q_OBJECT
protected:
    bool opened;
    int pageCount;
    QString fileName;
    virtual int getPageCountPrivate() = 0;

public:
    explicit ZAbstractReader(QObject *parent, QString filename);
    ~ZAbstractReader();
    bool openFile(QString filename);
    bool isOpened();
    int getPageCount();

    virtual bool openFile() = 0;
    virtual void closeFile();
    virtual QImage loadPage(int num) = 0;
    virtual QImageHash loadPages(QIntList nums) = 0;
    virtual QString getMagic() = 0;
    
signals:
    
public slots:
    
};

#endif // ZABSTRACTREADER_H
