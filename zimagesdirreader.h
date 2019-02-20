#ifndef ZIMAGESDIRREADER_H
#define ZIMAGESDIRREADER_H

#include "zglobal.h"
#include "zabstractreader.h"

class ZImagesDirReader : public ZAbstractReader
{
public:
    explicit ZImagesDirReader(QObject *parent, const QString &filename);
    bool openFile();
    void closeFile();
    QByteArray loadPage(int num);
    QString getMagic();
    QString getInternalPath(int idx);

};

#endif // ZIMAGESDIRREADER_H
