#ifndef ZGOOGLEVISION_H
#define ZGOOGLEVISION_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QSize>
#include <QJsonObject>
#include "abstractocr.h"

class ZGoogleVision : public ZAbstractOCR
{
    Q_OBJECT
public:
    explicit ZGoogleVision(QObject *parent = nullptr);

    bool isReady() const override;
    void processImage(const QImage& image) override;
    void processPage(const QImage &image, int pageNumber) override;
    bool supportsFullPage() const override;

private:
    QString m_gcpProject;
    QString m_gcpEmail;
    QString m_gcpPrivateKey;
    QString m_gcpPrivateKeyID;
    QString m_authHeader;
    QByteArray m_preparedImage;
    QNetworkAccessManager m_nam;

    bool m_pageMode = false;
    int m_pageNumber = -1;
    QSize m_originalImageSize;
    QSize m_uploadedImageSize;

    bool parseJsonKey(const QString &jsonKeyFile);
    void gcpAuth();
    void startVision();

    bool prepareImage(const QImage &image);
    Z::ZOCRPageResult parsePageResult(
        const QJsonObject &response) const;

};

#endif // ZGOOGLEVISION_H
