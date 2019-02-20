#ifndef ZPDFREADER_H
#define ZPDFREADER_H

#ifdef WITH_POPPLER
#include <poppler/PDFDoc.h>
#endif

#include "zabstractreader.h"
#include "zpdfimageoutdev.h"

class ZPdfReader : public ZAbstractReader
{
    Q_OBJECT

#ifdef WITH_POPPLER
    PDFDoc* doc;
    ZPDFImageOutputDev* outDev;
#endif
    bool useImageCatalog;
    int numPages;

public:
    explicit ZPdfReader(QObject *parent, const QString &filename);
    ~ZPdfReader();
    bool openFile();
    void closeFile();
    QByteArray loadPage(int num);
    QString getMagic();
    
};

void initPdfReader();
void freePdfReader();

#endif // ZPDFREADER_H
