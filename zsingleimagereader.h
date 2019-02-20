#ifndef ZSINGLEIMAGEREADER_H
#define ZSINGLEIMAGEREADER_H

#include <QObject>
#include <QImage>
#include "zabstractreader.h"

class ZSingleImageReader : public ZAbstractReader
{
    Q_OBJECT
protected:
    QImage page;
public:
    explicit ZSingleImageReader(QObject *parent, const QString &filename);
    bool openFile();
    void closeFile();
    QByteArray loadPage(int num);
    QString getMagic();
    QString getInternalPath(int idx);
};

#endif // ZSINGLEIMAGEREADER_H
