#ifndef PADDLEOCR_H
#define PADDLEOCR_H

#include <QObject>
#include <QNetworkAccessManager>
#include "abstractocr.h"

class ZPaddleOCR : public ZAbstractOCR
{
    Q_OBJECT
public:
    explicit ZPaddleOCR(QObject *parent = nullptr);

    bool isReady() const override;
    void processImage(const QImage &image) override;
    bool supportsFullPage() const override { return true; }
    void processPage(const QImage &image, int pageNumber) override;

private:
    void submit(const QImage &image, int pageNumber, bool pageMode);
    bool parseResult(const QJsonObject &root,
                     const QSize &imageSize,
                     QString &error, Z::ZOCRPageResult &ocrResult);
    void reportError(const QString &message);

    QNetworkAccessManager m_nam;
};


#endif // PADDLEOCR_H
