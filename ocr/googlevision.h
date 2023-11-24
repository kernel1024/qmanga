#ifndef ZGOOGLEVISION_H
#define ZGOOGLEVISION_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include "abstractocr.h"

class ZGoogleVision : public ZAbstractOCR
{
    Q_OBJECT
public:
    explicit ZGoogleVision(QObject *parent = nullptr);

    bool isReady() const override;
    void processImage(const QImage& image) override;

private:
    QString m_gcpProject;
    QString m_gcpEmail;
    QString m_gcpPrivateKey;
    QString m_gcpPrivateKeyID;
    QString m_authHeader;
    QByteArray m_preparedImage;
    QNetworkAccessManager m_nam;

    bool parseJsonKey(const QString &jsonKeyFile);
    void gcpAuth();
    void startVision();

};

#endif // ZGOOGLEVISION_H
