#include <QJsonValue>
#include <QJsonArray>
#include <QPolygonF>
#include <QNetworkReply>

#include "paddleocr.h"
#include "../global.h"
#include "../zglobal.h"

namespace ZDefaults {
const int paddleXJpegQuality = 95;
const int paddleXConnectionTimeoutMS = 120000;
}

namespace {

QPolygonF parsePolygon(const QJsonValue &value)
{
    QPolygonF polygon;
    if (!value.isArray())
        return polygon;

    for (const auto &pointValue : value.toArray()) {
        if (!pointValue.isArray())
            return {};
        const QJsonArray point = pointValue.toArray();
        if (point.size() != 2 || !point.at(0).isDouble() ||
            !point.at(1).isDouble())
            return {};
        polygon.append(QPointF(point.at(0).toDouble(),
                               point.at(1).toDouble()));
    }
    return polygon.size() >= 3 ? polygon : QPolygonF{};
}

}

ZPaddleOCR::ZPaddleOCR(QObject *parent)
    : ZAbstractOCR{parent}
{

}

bool ZPaddleOCR::isReady() const
{
    const QUrl endpoint(zF->global()->getPaddleXUrl());
    return endpoint.isValid() &&
           (endpoint.scheme() == QLatin1String("http") ||
            endpoint.scheme() == QLatin1String("https")) &&
           !endpoint.host().isEmpty();
}

void ZPaddleOCR::processImage(const QImage &image)
{
    submit(image, -1, false);
}

void ZPaddleOCR::processPage(const QImage &image, int pageNumber)
{
    submit(image, pageNumber, true);
}

void ZPaddleOCR::reportError(const QString &message)
{
    setError(message);
    // ZAbstractOCR currently has no dedicated error signal.
    // Keep compatibility with existing OCR editor error reporting.
    Q_EMIT gotOCRResult(message);
}

bool ZPaddleOCR::parseResult(const QJsonObject &root, const QSize &imageSize, QString &error, Z::ZOCRPageResult &ocrResult)
{
    Z::ZOCRPageResult res;
    res.imageSize = imageSize;

    if (root.value(QSL("errorCode")).toInt(-1) != 0) {
        error = root.value(QSL("errorMsg"))
        .toString(QSL("PaddleX inference error"));
        ocrResult = res;
        return false;
    }

    const QJsonObject result = root.value(QSL("result")).toObject();
    const QJsonArray ocrResults = result.value(QSL("ocrResults")).toArray();
    if (ocrResults.isEmpty()) {
        error = QSL("PaddleX returned no OCR result entries");
        ocrResult = res;
        return false;
    }

    for (const auto &entryValue : ocrResults) {
        if (!entryValue.isObject())
            continue;
        const QJsonObject data = entryValue.toObject()
                                     .value(QSL("prunedResult")).toObject();
        const QJsonArray texts = data.value(QSL("rec_texts")).toArray();
        const QJsonArray scores = data.value(QSL("rec_scores")).toArray();
        const QJsonArray polys = data.value(QSL("rec_polys")).toArray();

        if (texts.size() != polys.size()) {
            error = QSL("PaddleX OCR text/polygon count mismatch");
            ocrResult = res;
            return false;
        }

        for (qsizetype i = 0; i < texts.size(); ++i) {
            Z::ZOCRTextRegion region;
            region.text = texts.at(i).toString().trimmed();
            region.polygon = parsePolygon(polys.at(i));
            region.confidence = i < scores.size() && scores.at(i).isDouble()
                                    ? float(scores.at(i).toDouble()) : -1.0F;

            if (!region.text.isEmpty() && region.polygon.size() >= 3)
                res.regions.append(region);
        }
    }
    ocrResult = res;
    return true;
}

void ZPaddleOCR::submit(const QImage &image, int pageNumber, bool pageMode)
{
    if (!isReady()) {
        reportError(tr("PaddleX OCR: invalid server URL"));
        return;
    }
    if (image.isNull()) {
        reportError(tr("PaddleX OCR: empty input image"));
        return;
    }

    QByteArray bytes;
    QBuffer buffer(&bytes);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "JPG", ZDefaults::paddleXJpegQuality)) {
        reportError(tr("PaddleX OCR: image encoding failed"));
        return;
    }

    QJsonObject payload;
    payload.insert(QSL("file"),
                   QString::fromLatin1(bytes.toBase64()));
    payload.insert(QSL("fileType"), 1);
    payload.insert(QSL("visualize"), false);
    payload.insert(QSL("useDocOrientationClassify"), false);
    payload.insert(QSL("useDocUnwarping"), false);
    payload.insert(QSL("useTextlineOrientation"), false);

    const QUrl endpoint(zF->global()->getPaddleXUrl());
    QNetworkRequest request(endpoint);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QSL("application/json"));
    request.setTransferTimeout(ZDefaults::paddleXConnectionTimeoutMS);
    const QByteArray body = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    QNetworkReply *reply = m_nam.post(request, body);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, pageNumber, pageMode, imageSize = image.size()] {
                const QByteArray response = reply->readAll();
                const auto networkError = reply->error();
                const QString networkErrorText = reply->errorString();
                reply->deleteLater();

                if (networkError != QNetworkReply::NoError) {
                    reportError(tr("PaddleX OCR: network error: %1").arg(networkErrorText));
                    return;
                }

                QJsonParseError parseError{};
                const QJsonDocument document = QJsonDocument::fromJson(response, &parseError);
                if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
                    reportError(tr("PaddleX OCR: invalid JSON response: %1")
                                    .arg(parseError.errorString()));
                    return;
                }

                QString error;
                Z::ZOCRPageResult result;
                if (!parseResult(document.object(), imageSize, error, result)) {
                    reportError(tr("PaddleX OCR: %1").arg(error));
                    return;
                }

                if (pageMode) {
                    Q_EMIT gotOCRPageResult(result, pageNumber);
                } else {
                    QStringList lines;
                    for (const auto &region : std::as_const(result.regions))
                        lines.append(region.text);
                    Q_EMIT gotOCRResult(lines.join(QLatin1Char('\n')));
                }
            });
}
