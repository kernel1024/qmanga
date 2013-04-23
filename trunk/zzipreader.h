#ifndef ZZIPREADER_H
#define ZZIPREADER_H

#include <zzip/zzip.h>
#include "zglobal.h"
#include "zabstractreader.h"

class ZZipReader : public ZAbstractReader
{
protected:
    ZZIP_DIR* mainZFile;

public:
    explicit ZZipReader(QObject *parent, QString filename);
    bool openFile();
    void closeFile();
    QByteArray loadPage(int num);
    QString getMagic();
    QString getInternalPath(int idx);
};

#endif // ZZIPREADER_H
