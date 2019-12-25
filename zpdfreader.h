#ifndef ZPDFREADER_H
#define ZPDFREADER_H

#ifdef WITH_POPPLER
#include <poppler/PDFDoc.h>
#endif

#include <QMutex>
#include "zabstractreader.h"
#include "global.h"

class ZPDFImg {
public:
    int pos { 0 };
    int size { 0 };
    int width { 0 };
    int height { 0 };
    Z::PDFImageFormat format { Z::imgUndefined };

    ZPDFImg() = default;
    ~ZPDFImg() = default;
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
public:
    ZPdfReader(QObject *parent, const QString &filename);
    ~ZPdfReader() override;
    bool openFile() override;
    void closeFile() override;
    QByteArray loadPage(int num) override;
    QImage loadPageImage(int num) override;
    QString getMagic() override;

private:
    bool m_useImageCatalog { false };
    QMutex m_indexerMutex;
    QList<ZPDFImg> m_images;
    int zlibInflate(const char *src, int srcSize, uchar *dst, int dstSize);

#ifdef WITH_POPPLER
    PDFDoc* m_doc;
    void loadPagePrivate(int num, QByteArray *buf, QImage *img, bool preferImage);
#endif

    Q_DISABLE_COPY(ZPdfReader)

};

class ZPdfController : public QObject
{
    Q_OBJECT
public:
    ZPdfController(QObject *parent = nullptr);
    ~ZPdfController() override;
};

#endif // ZPDFREADER_H
