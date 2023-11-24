#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QUrlQuery>
#include <QNetworkReply>

#include "../global.h"
#include "../zglobal.h"
#include "googlevision.h"

namespace CDefaults {
const int maxImageSide = 2000;
const int imageQuality = 96;
const int oneHour = 60 * 60;
const auto imageFormat = "JPG";
}

ZGoogleVision::ZGoogleVision(QObject *parent)
    : ZAbstractOCR{parent}
{

}

bool ZGoogleVision::isReady() const
{
    return true; // TODO: need something
}

void ZGoogleVision::processImage(const QImage &image)
{
    if (!m_preparedImage.isEmpty())
        return;

    QImage img = image;
    if (img.width() > img.height()) {
        if (img.width() > CDefaults::maxImageSide) {
            img = img.scaledToWidth(CDefaults::maxImageSide, Qt::SmoothTransformation);
        }
    } else {
        if (img.height() > CDefaults::maxImageSide) {
            img = img.scaledToHeight(CDefaults::maxImageSide, Qt::SmoothTransformation);
        }
    }
    QByteArray ba;
    QBuffer buf(&ba);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, CDefaults::imageFormat, CDefaults::imageQuality);
    buf.close();

    m_preparedImage = ba.toBase64();

    if (m_authHeader.isEmpty()) {
        if (parseJsonKey(zF->global()->getGCPKeyFile())) {
            gcpAuth();
            return;
        }
        // key parse error, message emitted, just leave
        m_preparedImage.clear();
        return;
    }

    // already have auth
    startVision();
}


bool ZGoogleVision::parseJsonKey(const QString &jsonKeyFile)
{
    QFile f(jsonKeyFile);
    if (!f.open(QIODevice::ReadOnly)) {
        Q_EMIT gotOCRResult(tr("Google Vision: cannot open JSON key file %1").arg(jsonKeyFile));
        return false;
    }

    QJsonParseError err{};
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();
    if (doc.isNull()) {
        Q_EMIT gotOCRResult(
            tr("Google Vision: JSON key parsing error #%1: %2").arg(err.error).arg(err.offset));
        return false;
    }

    m_gcpProject = doc.object().value(QSL("project_id")).toString();
    m_gcpEmail = doc.object().value(QSL("client_email")).toString();
    m_gcpPrivateKey = doc.object().value(QSL("private_key")).toString();
    m_gcpPrivateKeyID = doc.object().value(QSL("private_key_id")).toString();
    m_authHeader.clear();

    return true;
}

void ZGoogleVision::gcpAuth()
{
    m_authHeader.clear();

    if (m_gcpPrivateKey.isEmpty())
        return;

    const QString scope = QSL("https://www.googleapis.com/auth/cloud-platform "
                              "https://www.googleapis.com/auth/cloud-vision");
    const QString aud = QSL("https://oauth2.googleapis.com/token");

    const QDateTime time = QDateTime::currentDateTimeUtc();
    const QString iat = QSL("%1").arg(time.toSecsSinceEpoch());
    const QString exp = QSL("%1").arg(time.addSecs(CDefaults::oneHour).toSecsSinceEpoch());

    const QString jwtHeader = QSL(R"({"alg":"RS256","typ":"JWT"})");
    const QString jwtClaimSet = QSL(R"({"iss":"%1","scope":"%2","aud":"%3","exp":%4,"iat":%5})")
                                    .arg(m_gcpEmail, scope, aud, exp, iat);

    QByteArray jwt = jwtHeader.toUtf8().toBase64(QByteArray::Base64UrlEncoding);
    jwt.append(u'.');
    jwt.append(jwtClaimSet.toUtf8().toBase64(QByteArray::Base64UrlEncoding));

    QByteArray sign = ZGenericFuncs::signSHA256withRSA(jwt, m_gcpPrivateKey.toLatin1());
    if (sign.isEmpty())
        return;

    jwt.append(u'.');
    jwt.append(sign.toBase64(QByteArray::Base64UrlEncoding));

    QUrlQuery uq;
    uq.addQueryItem(QSL("grant_type"), QSL("urn:ietf:params:oauth:grant-type:jwt-bearer"));
    uq.addQueryItem(QSL("assertion"), QString::fromLatin1(jwt));
    QUrl rqurl = QUrl(aud);
    rqurl.setQuery(uq);
    QNetworkRequest rq(rqurl);
    rq.setRawHeader("Content-Type", "application/x-www-form-urlencoded");

    QNetworkReply *rpl = m_nam.post(rq, QByteArray());

    connect(rpl, &QNetworkReply::errorOccurred, this, [this, rpl](QNetworkReply::NetworkError error) {
        Q_UNUSED(error)

        Q_EMIT gotOCRResult(QSL("Google Vision: network error"));
        rpl->deleteLater();
    });

    connect(rpl, &QNetworkReply::finished, this, [this, rpl]() {
        QByteArray ra = rpl->readAll();
        QJsonParseError err{};
        QJsonDocument doc = QJsonDocument::fromJson(ra, &err);
        if (doc.isNull()) {
            Q_EMIT gotOCRResult(tr("Google Vision: Cloud auth JSON reply error #%1: %2")
                                    .arg(err.error)
                                    .arg(err.offset));
            m_authHeader.clear();
        } else {
            m_authHeader = QSL("Bearer %1").arg(doc.object().value(QSL("access_token")).toString());
        }
        rpl->deleteLater();

        startVision();
    });
}

void ZGoogleVision::startVision()
{
    if (m_preparedImage.isEmpty())
        return;

    const QString apiKey = zF->global()->getGCPApiKey();

    if (m_gcpProject.isEmpty() || m_authHeader.isEmpty() || apiKey.isEmpty()) {
        Q_EMIT gotOCRResult(tr("Google Vision: Google Cloud auth failure"));
        return;
    }

    QUrl rqurl = QUrl(QSL("https://vision.googleapis.com/v1/images:annotate"));
    QUrlQuery rqKey;
    rqKey.addQueryItem(QSL("key"), apiKey);
    rqurl.setQuery(rqKey);

    QJsonObject image;
    image[QSL("content")] = QString::fromUtf8(m_preparedImage);

    QJsonObject features;
    features[QSL("type")] = QSL("TEXT_DETECTION");
    features[QSL("maxResults")] = 1;

    QJsonObject request;
    request[QSL("image")] = image;
    request[QSL("features")] = features;

    QJsonObject requests;
    requests[QSL("requests")] = QJsonArray({request});

    QJsonDocument doc(requests);
    QByteArray body = doc.toJson(QJsonDocument::Compact);

    QNetworkRequest rq(rqurl);
    rq.setRawHeader("Authorization", m_authHeader.toUtf8());
    rq.setRawHeader("Content-Type", "application/json; charset=utf-8");
    rq.setRawHeader("Content-Length", QString::number(body.size()).toLatin1());

    QNetworkReply *rpl = m_nam.post(rq, body);

    connect(rpl, &QNetworkReply::errorOccurred, this, [this, rpl](QNetworkReply::NetworkError error) {
        Q_UNUSED(error)

        Q_EMIT gotOCRResult(QSL("Google Vision: network error during text detection"));
        rpl->deleteLater();
    });

    connect(rpl, &QNetworkReply::finished, this, [this, rpl]() {
        QByteArray ra = rpl->readAll();
        QJsonParseError err{};
        QJsonDocument doc = QJsonDocument::fromJson(ra, &err);
        if (doc.isNull()) {
            Q_EMIT gotOCRResult(tr("Google Vision: Cloud response JSON reply error #%1: %2")
                                    .arg(err.error)
                                    .arg(err.offset));
        } else {
            QString res;
            doc = QJsonDocument::fromJson(ra);
            if (doc.isNull())
                res = tr("ERROR: Google Vision JSON error");

            if (res.isEmpty() && !doc.isObject())
                res = tr("ERROR: Google Vision JSON generic error");

            QJsonValue err = doc.object().value(QSL("error"));
            if (err.isObject()) {
                res = tr("ERROR: Google Vision JSON error #%1: %2")
                          .arg(err.toObject().value(QSL("code")).toInt())
                          .arg(err.toObject().value(QSL("message")).toString());
            }

            const QJsonArray responses = doc.object().value(QSL("responses")).toArray();
            for (const auto &resp : responses) {
                res += resp.toObject()
                           .value(QSL("fullTextAnnotation"))
                           .toObject()
                           .value(QSL("text"))
                           .toString();
            }

            if (!res.isEmpty())
                Q_EMIT gotOCRResult(res);
        }
        m_preparedImage.clear();
        rpl->deleteLater();
    });
}
