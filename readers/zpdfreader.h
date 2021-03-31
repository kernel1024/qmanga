#ifndef ZPDFREADER_H
#define ZPDFREADER_H

#ifdef WITH_POPPLER
#include <poppler/PDFDoc.h>

#include <poppler/cpp/poppler-version.h>
#if POPPLER_VERSION_MAJOR==21
    #if POPPLER_VERSION_MINOR<3
        #define ZPDF_PRE2103_API 1
    #endif
#endif

#endif // WITH_POPPLER

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
    Q_DISABLE_COPY(ZPdfReader)
public:
    ZPdfReader(QObject *parent, const QString &filename);
    ~ZPdfReader() override;
    bool openFile() override;
    void closeFile() override;
    QByteArray loadPage(int num) override;
    QImage loadPageImage(int num) override;
    QString getMagic() override;

    static bool preloadFile(const QString &filename, bool preserveDocument);

private:
    bool m_useImageCatalog { false };
    QMutex m_indexerMutex;
    QList<ZPDFImg> m_images;
    int zlibInflate(const char *src, int srcSize, uchar *dst, int dstSize);

#ifdef WITH_POPPLER

#ifdef ZPDF_PRE2103_API
    PDFDoc* m_doc { nullptr };
#else
    std::unique_ptr<PDFDoc> m_doc;
#endif

    void loadPagePrivate(int num, QByteArray *buf, QImage *img, bool preferImage);
#endif
};

class ZPdfController : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ZPdfController)
private:
    QHash<QString,QString> m_officeDocs;
    void cleanTmpFiles();

public:
    ZPdfController(QObject *parent = nullptr);
    ~ZPdfController() override;

    QString getConvertedDocumentPDF(const QString& sourceFilename) const;
    void addConvertedDocumentPDF(const QString& sourceFilename, const QString& pdfFile);
};

#endif // ZPDFREADER_H
