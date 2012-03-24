#ifndef ZRARREADER_H
#define ZRARREADER_H

#include <QtCore>
#include "zabstractreader.h"

class ZRarReader : public ZAbstractReader
{
    Q_OBJECT
private:
    int getPageCountPrivate();
    QList<ZFileEntry> sortList;
    QString rarExec;

public:
    explicit ZRarReader(QObject *parent, QString filename);
    bool openFile();
    void closeFile();
    QImage loadPage(int num);
    QImageHash loadPages(QIntList nums);
    QString getMagic();

};

#endif // ZRARREADER_H
