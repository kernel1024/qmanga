#ifndef ZPDFREADER_H
#define ZPDFREADER_H

#ifdef WITH_POPPLER
#include <poppler/PDFDoc.h>
#endif

#include <QMutex>
#include "zabstractreader.h"

class ZPDFImg {
public:
    int pos, size, width, height;
    Z::PDFImageFormat format;

    ZPDFImg();
    ZPDFImg(const ZPDFImg& other);
    ZPDFImg(int a_pos, int a_size, int a_width, int a_height,
            Z::PDFImageFormat a_format);
    ZPDFImg &operator=(const ZPDFImg& other) = default;
    bool operator==(const ZPDFImg& ref) const;
    bool operator!=(const ZPDFImg& ref) const;
};

class ZPdfReader : public ZAbstractReader
{
    Q_OBJECT
private:

    bool useImageCatalog;
    int numPages;
    QMutex indexerMutex;
    QList<ZPDFImg> images;
    int zlibInflate(const char *src, int srcSize, uchar *dst, int dstSize);

#ifdef WITH_POPPLER
    PDFDoc* doc;
    void loadPagePrivate(int num, QByteArray *buf, QImage *img, bool preferImage);
#endif

public:
    explicit ZPdfReader(QObject *parent, const QString &filename);
    ~ZPdfReader();
    bool openFile();
    void closeFile();
    QByteArray loadPage(int num);
    QImage loadPageImage(int num);
    QString getMagic();
    
};

void initPdfReader();
void freePdfReader();

#endif // ZPDFREADER_H
