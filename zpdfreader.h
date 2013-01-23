#ifndef ZPDFREADER_H
#define ZPDFREADER_H

#include <QtCore>

#ifdef WITH_POPPLER
#include <poppler-document.h>
#include <poppler-page-renderer.h>
#endif

#include "zabstractreader.h"

class ZPdfReader : public ZAbstractReader
{
    Q_OBJECT
protected:
#ifdef WITH_POPPLER
    poppler::document* doc;
    poppler::page_renderer* renderer;
#endif

public:
    explicit ZPdfReader(QObject *parent, QString filename);
    ~ZPdfReader();
    bool openFile();
    void closeFile();
    QByteArray loadPage(int num);
    QByteHash loadPages(QIntList nums);
    QString getMagic();
    
};

#endif // ZPDFREADER_H
