#ifndef ZRARREADER_H
#define ZRARREADER_H

#include <QtCore>
#include "zabstractreader.h"

class ZRarReader : public ZAbstractReader
{
    Q_OBJECT
protected:
    QString rarExec;

public:
    explicit ZRarReader(QObject *parent, QString filename);
    bool openFile();
    void closeFile();
    QImage loadPage(int num);
    QImageHash loadPages(QIntList nums);
    QString getMagic();
    QString getInternalPath(int idx);

};

#endif // ZRARREADER_H
