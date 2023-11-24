#ifndef ZTESSERACTOCR_H
#define ZTESSERACTOCR_H

#include <QObject>

#ifdef WITH_TESSERACT
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#else // WITH_TESSERACT
typedef void PIX;
#endif

#include "abstractocr.h"

class ZTesseractOCR : public ZAbstractOCR
{
    Q_OBJECT
public:
    explicit ZTesseractOCR(QObject *parent = nullptr);

    static QString tesseractDatapath();
    static QString tesseractGetActiveLanguage();
    static void preinit();

    bool isReady() const override;
    void processImage(const QImage& image) override;

private:
#ifdef WITH_TESSERACT
    static tesseract::TessBaseAPI* m_tesseract;
#endif

    static bool m_initFailed;
    static PIX* Image2PIX(const QImage& qImage);

};

#endif // ZTESSERACTOCR_H
