#ifndef ZZIPREADER_H
#define ZZIPREADER_H

#include <QtCore>
#include <QtGui>
#include <quazip/quazip.h>
#include <quazip/quazipfile.h>
#include "zglobal.h"
#include "zabstractreader.h"

class ZZipReader : public ZAbstractReader
{
protected:
    QuaZip mainZFile;
    QFile mainFile;

public:
    explicit ZZipReader(QObject *parent, QString filename);
    bool openFile();
    void closeFile();
    QByteArray loadPage(int num);
    QByteHash loadPages(QIntList nums);
    QString getMagic();
    QString getInternalPath(int idx);
};

#endif // ZZIPREADER_H
