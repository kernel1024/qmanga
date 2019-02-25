#ifndef ZZIPREADER_H
#define ZZIPREADER_H

#include <zip.h>
#include "zabstractreader.h"

class ZZipReader : public ZAbstractReader
{
private:
    zip_t* mainZFile;
    QHash <int,int> m_sizes;

public:
    explicit ZZipReader(QObject *parent, const QString &filename);
    bool openFile();
    void closeFile();
    QByteArray loadPage(int num);
    QImage loadPageImage(int num);
    QString getMagic();
    QString getInternalPath(int idx);
};

#endif // ZZIPREADER_H
