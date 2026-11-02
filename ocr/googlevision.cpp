#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QUrlQuery>
#include <QNetworkReply>
#include <QPolygonF>
#include <QTransform>

#include "../global.h"
#include "../zglobal.h"
#include "googlevision.h"

namespace CDefaults {
const int maxImageSide = 2000;
const int imageQuality = 96;
const int oneHour = 60 * 60;
const auto imageFormat = "JPG";
}

namespace {

QPolygonF parseBoundingPoly(
    const QJsonObject &object)
{
    QPolygonF polygon;

    const QJsonArray vertices =
        object.value(QSL("vertices")).toArray();

    for (const auto &value : vertices) {
        const QJsonObject vertex = value.toObject();

        polygon.append(QPointF(
            vertex.value(QSL("x")).toDouble(),
            vertex.value(QSL("y")).toDouble()
            ));
    }

    return polygon;
}

QString parseParagraphText(
    const QJsonObject &paragraph)
{
    QString text;

    const QJsonArray words =
        paragraph.value(QSL("words")).toArray();

    for (const auto &wordValue : words) {
        const QJsonObject word = wordValue.toObject();

        const QJsonArray symbols =
            word.value(QSL("symbols")).toArray();

        for (const auto &symbolValue : symbols) {
            const QJsonObject symbol =
                symbolValue.toObject();

            text += symbol.value(
                              QSL("text")).toString();

            const QJsonObject properties =
                symbol.value(
                          QSL("property")).toObject();

            const QJsonObject detectedBreak =
                properties.value(
                              QSL("detectedBreak")).toObject();

            const QString breakType =
                detectedBreak.value(
                                 QSL("type")).toString();

            if (breakType == QSL("SPACE") ||
                breakType == QSL("SURE_SPACE")) {

                text += QLatin1Char(' ');

            } else if (
                breakType == QSL("EOL_SURE_SPACE")) {

                text += QLatin1Char(' ');

            } else if (
                breakType == QSL("LINE_BREAK")) {

                text += QLatin1Char('\n');

            } else if (
                breakType == QSL("HYPHEN")) {

                // Google indicates a hyphenated line break.
                // No extra separator is needed here.
            }
        }
    }

    return text.trimmed();
}

bool shouldJoinWithoutSeparator(
    const QString &left,
    const QString &right)
{
    if (left.isEmpty() || right.isEmpty())
        return true;

    const auto isCJK = [](QChar ch) {
        switch (ch.script()) {
        case QChar::Script_Han:
        case QChar::Script_Hiragana:
        case QChar::Script_Katakana:
            return true;

        default:
            return false;
        }
    };

    return isCJK(left.back()) || isCJK(right.front());
}

} // namespace


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
    if (!prepareImage(image))
        return;

    m_pageMode = false;
    m_pageNumber = -1;

    if (m_authHeader.isEmpty()) {
        if (parseJsonKey(
                zF->global()->getGCPKeyFile())) {
            gcpAuth();
            return;
        }

        m_preparedImage.clear();
        return;
    }

    startVision();
}

bool ZGoogleVision::supportsFullPage() const
{
    return true;
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
            rpl->deleteLater();
            return;
        }

        const QString token =
            doc.object()
                .value(QSL("access_token"))
                .toString();

        if (token.isEmpty()) {
            m_authHeader.clear();
            m_preparedImage.clear();
            m_pageMode = false;
            m_pageNumber = -1;

            setError(
                tr("Google Vision: authentication failed")
                );
        }

        m_authHeader = QSL("Bearer %1").arg(token);

        rpl->deleteLater();
        startVision();
    });
}

void ZGoogleVision::startVision()
{
    if (m_preparedImage.isEmpty())
        return;

    const QString apiKey =
        zF->global()->getGCPApiKey();

    if (m_gcpProject.isEmpty() ||
        m_authHeader.isEmpty() ||
        apiKey.isEmpty()) {

        const QString error =
            tr("Google Vision: Google Cloud auth failure");

        setError(error);
        Q_EMIT gotOCRResult(error);

        m_preparedImage.clear();
        m_pageMode = false;
        m_pageNumber = -1;
        return;
    }

    QUrl rqurl(
        QSL("https://vision.googleapis.com/v1/images:annotate")
        );

    QUrlQuery rqKey;
    rqKey.addQueryItem(QSL("key"), apiKey);
    rqurl.setQuery(rqKey);

    // Image data.
    QJsonObject image;
    image[QSL("content")] =
        QString::fromUtf8(m_preparedImage);

    // Select recognition type.
    QJsonObject features;

    features[QSL("type")] =
        m_pageMode
            ? QSL("DOCUMENT_TEXT_DETECTION")
            : QSL("TEXT_DETECTION");

    // Build request.
    QJsonObject request;
    request[QSL("image")] = image;
    request[QSL("features")] = QJsonArray{features};

    QJsonObject requests;
    requests[QSL("requests")] = QJsonArray{request};

    const QByteArray body =
        QJsonDocument(requests).toJson(
            QJsonDocument::Compact
            );

    QNetworkRequest rq(rqurl);

    rq.setRawHeader(
        "Authorization",
        m_authHeader.toUtf8()
        );

    rq.setRawHeader(
        "Content-Type",
        "application/json; charset=utf-8"
        );

    // Remember the type of this specific request.
    const bool pageMode = m_pageMode;

    QNetworkReply *rpl = m_nam.post(rq, body);

    connect(
        rpl,
        &QNetworkReply::finished,
        this,
        [this, rpl, pageMode]()
        {
            const QByteArray ra = rpl->readAll();

            // Release the busy state regardless of outcome.
            m_preparedImage.clear();
            m_pageMode = false;
            m_pageNumber = -1;

            const auto reportError =
                [this](const QString &error)
            {
                setError(error);

                // Retain the existing error-reporting
                // convention for compatibility.
                Q_EMIT gotOCRResult(error);
            };

            if (rpl->error() !=
                QNetworkReply::NoError) {

                reportError(
                    tr("Google Vision: network error: %1")
                        .arg(rpl->errorString())
                    );

                rpl->deleteLater();
                return;
            }

            QJsonParseError err{};

            const QJsonDocument doc =
                QJsonDocument::fromJson(ra, &err);

            if (err.error != QJsonParseError::NoError ||
                !doc.isObject()) {

                reportError(
                    tr("Google Vision: JSON reply error: %1")
                        .arg(err.errorString())
                    );

                rpl->deleteLater();
                return;
            }

            const QJsonObject root = doc.object();

            // Top-level Google API error.
            const QJsonObject apiError =
                root.value(QSL("error")).toObject();

            if (!apiError.isEmpty()) {
                reportError(
                    tr("Google Vision: API error #%1: %2")
                        .arg(
                            apiError.value(
                                        QSL("code")).toInt()
                            )
                        .arg(
                            apiError.value(
                                        QSL("message")).toString()
                            )
                    );

                rpl->deleteLater();
                return;
            }

            const QJsonArray responses =
                root.value(
                        QSL("responses")).toArray();

            if (responses.isEmpty()) {
                reportError(
                    tr("Google Vision: empty API response")
                    );

                rpl->deleteLater();
                return;
            }

            // Check all individual image responses.
            for (const auto &value : responses) {
                const QJsonObject response =
                    value.toObject();

                const QJsonObject error =
                    response.value(
                                QSL("error")).toObject();

                if (!error.isEmpty()) {
                    reportError(
                        tr("Google Vision: %1")
                            .arg(
                                error.value(
                                         QSL("message")).toString()
                                )
                        );

                    rpl->deleteLater();
                    return;
                }
            }

            if (pageMode) {

                // Full-page mode:
                // return individual text regions.
                Z::ZOCRPageResult result;
                result.imageSize = m_originalImageSize;

                for (const auto &value : responses) {
                    const Z::ZOCRPageResult page =
                        parsePageResult(value.toObject());

                    result.regions.append(page.regions);
                }

                Q_EMIT gotOCRPageResult(result, m_pageNumber);

            } else {

                // Legacy selection mode:
                // return the entire text as QString.
                QString text;

                for (const auto &value : responses) {
                    text += value.toObject()
                    .value(
                        QSL("fullTextAnnotation"))
                        .toObject()
                        .value(QSL("text"))
                        .toString();
                }

                if (!text.isEmpty())
                    Q_EMIT gotOCRResult(text);
            }

            rpl->deleteLater();
        }
    );
}

bool ZGoogleVision::prepareImage(const QImage &image)
{
    if (!m_preparedImage.isEmpty())
        return false;

    if (image.isNull())
        return false;

    m_originalImageSize = image.size();

    QImage img = image;

    if (img.width() > img.height()) {
        if (img.width() > CDefaults::maxImageSide) {
            img = img.scaledToWidth(
                CDefaults::maxImageSide,
                Qt::SmoothTransformation
                );
        }
    } else {
        if (img.height() > CDefaults::maxImageSide) {
            img = img.scaledToHeight(
                CDefaults::maxImageSide,
                Qt::SmoothTransformation
                );
        }
    }

    m_uploadedImageSize = img.size();

    QByteArray ba;
    QBuffer buf(&ba);

    if (!buf.open(QIODevice::WriteOnly))
        return false;

    if (!img.save(
            &buf,
            CDefaults::imageFormat,
            CDefaults::imageQuality)) {
        return false;
    }

    m_preparedImage = ba.toBase64();

    return !m_preparedImage.isEmpty();
}

void ZGoogleVision::processPage(const QImage &image, int pageNumber)
{
    if (!prepareImage(image))
        return;

    m_pageMode = true;
    m_pageNumber = pageNumber;

    if (m_authHeader.isEmpty()) {
        if (parseJsonKey(
                zF->global()->getGCPKeyFile())) {
            gcpAuth();
            return;
        }

        // Key parsing failed.
        m_preparedImage.clear();
        m_pageMode = false;
        m_pageNumber = -1;
        return;
    }

    // Already authenticated.
    startVision();
}

Z::ZOCRPageResult ZGoogleVision::parsePageResult(
    const QJsonObject &response) const
{
    Z::ZOCRPageResult result;

    // Results always use coordinates from the
    // original image supplied by qmanga.
    result.imageSize = m_originalImageSize;

    if (m_uploadedImageSize.isEmpty() ||
        m_originalImageSize.isEmpty()) {
        return result;
    }

    const qreal sx =
        qreal(m_originalImageSize.width()) /
        m_uploadedImageSize.width();

    const qreal sy =
        qreal(m_originalImageSize.height()) /
        m_uploadedImageSize.height();

    QTransform transform;
    transform.scale(sx, sy);

    const QJsonObject annotation =
        response.value(
                    QSL("fullTextAnnotation")).toObject();

    const QJsonArray pages =
        annotation.value(QSL("pages")).toArray();

    for (const auto &pageValue : pages) {
        const QJsonObject page =
            pageValue.toObject();

        const QJsonArray blocks =
            page.value(QSL("blocks")).toArray();

        for (const auto &blockValue : blocks) {
            const QJsonObject block = blockValue.toObject();

            Z::ZOCRTextRegion region;

            const QJsonArray paragraphs =
                block.value(QSL("paragraphs")).toArray();

            for (const auto &paragraphValue : paragraphs) {
                const QJsonObject paragraph =
                    paragraphValue.toObject();

                const QString text =
                    parseParagraphText(paragraph);

                if (text.isEmpty())
                    continue;

                if (!region.text.isEmpty() &&
                    !shouldJoinWithoutSeparator(region.text, text)) {
                    region.text += QLatin1Char('\n');
                }

                region.text += text;
            }

            if (region.text.isEmpty())
                continue;

            region.polygon = transform.map(
                parseBoundingPoly(
                    block.value(QSL("boundingBox")).toObject()
                    )
                );

            if (region.polygon.size() < 3)
                continue;

            region.confidence = static_cast<float>(
                block.value(QSL("confidence")).toDouble(-1.0)
                );

            result.regions.append(region);
        }

/*        for (const auto &blockValue : blocks) {
            const QJsonObject block =
                blockValue.toObject();

            const QJsonArray paragraphs =
                block.value(
                         QSL("paragraphs")).toArray();

            for (const auto &paragraphValue :
                 paragraphs) {

                const QJsonObject paragraph =
                    paragraphValue.toObject();

                Z::ZOCRTextRegion region;

                region.text =
                    parseParagraphText(paragraph);

                if (region.text.isEmpty())
                    continue;

                const QJsonObject boundingBox =
                    paragraph.value(
                                 QSL("boundingBox")).toObject();

                region.polygon = transform.map(
                    parseBoundingPoly(boundingBox)
                    );

                if (region.polygon.size() < 3)
                    continue;

                region.confidence =
                    static_cast<float>(
                        paragraph.value(
                                     QSL("confidence"))
                            .toDouble(-1.0)
                        );

                result.regions.append(region);
            }
        } */
    }

    return result;
}

