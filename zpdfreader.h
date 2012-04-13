#ifndef ZPDFREADER_H
#define ZPDFREADER_H

#include <QtCore>
#ifdef WITH_POPPLER
#include <poppler-qt4.h>
#endif
#include "zabstractreader.h"

class ZPdfReader : public ZAbstractReader
{
    Q_OBJECT
protected:
#ifdef WITH_POPPLER
    Poppler::Document* doc;
#endif

public:
    explicit ZPdfReader(QObject *parent, QString filename);
    bool openFile();
    void closeFile();
    QByteArray loadPage(int num);
    QByteHash loadPages(QIntList nums);
    QString getMagic();
    
};

#endif // ZPDFREADER_H
