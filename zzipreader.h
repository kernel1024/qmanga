#ifndef ZZIPREADER_H
#define ZZIPREADER_H

#include <QtCore>
#include <QtGui>
#include <quazip/quazip.h>
#include <quazip/quazipfile.h>
#include "zglobal.h"
#include "zabstractreader.h"

class ZFileEntry {
public:
    QString name;
    int idx;
    ZFileEntry();
    ZFileEntry(QString aName, int aIdx);
    ZFileEntry &operator=(const ZFileEntry& other);
    bool operator==(const ZFileEntry& ref) const;
    bool operator!=(const ZFileEntry& ref) const;
    bool operator<(const ZFileEntry& ref) const;
    bool operator>(const ZFileEntry& ref) const;
};

class ZZipReader : public ZAbstractReader
{
private:
    QuaZip mainZFile;
    QFile mainFile;
    int getPageCountPrivate();
    QList<ZFileEntry> sortList;

public:
    explicit ZZipReader(QObject *parent, QString filename);
    bool openFile();
    void closeFile();
    QImage loadPage(int num);
    QImageHash loadPages(QIntList nums);
    QString getMagic();
};

#endif // ZZIPREADER_H
