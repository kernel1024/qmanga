#ifndef ZPDFREADER_H
#define ZPDFREADER_H

#include <QtCore>
#include <poppler-qt4.h>
#include "zabstractreader.h"

class ZPdfReader : public ZAbstractReader
{
    Q_OBJECT
protected:
    Poppler::Document* doc;

public:
    explicit ZPdfReader(QObject *parent, QString filename);
    bool openFile();
    void closeFile();
    QByteArray loadPage(int num);
    QByteHash loadPages(QIntList nums);
    QString getMagic();
    
};

#endif // ZPDFREADER_H
