#include "ollamatranslator.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

#include "../global.h"

ZOllamaTranslator::ZOllamaTranslator(const QUrl &baseUrl, const QString &model,
                                     QObject *parent)
    : ZAbstractTranslator(parent), m_baseUrl(baseUrl), m_model(model)
{
}

bool ZOllamaTranslator::isSame(ZAbstractTranslator *src) const
{
    if (!src)
        return false;

    auto* psrc = qobject_cast<ZOllamaTranslator *>(src);

    if (!psrc)
        return false;

    if ((psrc->m_baseUrl != m_baseUrl) ||
        (psrc->m_model != m_model)) {
        return false;
    }

    return true;
}

bool ZOllamaTranslator::isReady() const
{
    // Configuration readiness, not an HTTP server health check.
    return m_baseUrl.isValid()
           && (m_baseUrl.scheme() == QStringLiteral("http")
               || m_baseUrl.scheme() == QStringLiteral("https"))
           && !m_baseUrl.host().isEmpty() && !m_model.trimmed().isEmpty();
}


void ZOllamaTranslator::startTranslation(quint64 requestId, const QString &text,
                                         const QString &sourceLang, const QString &targetLang)
{
    QJsonObject request;

    request[QSL("model")] = m_model;
    request[QSL("stream")] = false;

    request[QSL("prompt")] =
        QString(
            QSL("Translate the following text from %1 to %2. "
                "Preserve the meaning and dialogue structure. "
                "Return only the translation.\n\n%3")
            ).arg(sourceLang, targetLang, text);

    QNetworkRequest networkRequest(
        m_baseUrl.resolved(QUrl(QSL("/api/generate")))
        );

    networkRequest.setHeader(
        QNetworkRequest::ContentTypeHeader,
        QSL("application/json")
        );

    QNetworkReply *reply = m_network.post(
        networkRequest,
        QJsonDocument(request).toJson()
        );

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, requestId]
            {
                const QByteArray response = reply->readAll();

                if (reply->error() != QNetworkReply::NoError) {
                    finishTranslation(
                        requestId,
                        {},
                        reply->errorString()
                        );

                    reply->deleteLater();
                    return;
                }

                QJsonParseError parseError;

                const QJsonDocument document =
                    QJsonDocument::fromJson(response, &parseError);

                if (parseError.error != QJsonParseError::NoError ||
                    !document.isObject()) {

                    finishTranslation(
                        requestId,
                        {},
                        tr("Invalid Ollama JSON response.")
                        );

                    reply->deleteLater();
                    return;
                }

                const QString translation =
                    document.object()
                        .value(QSL("response"))
                        .toString();

                finishTranslation(requestId, translation);

                reply->deleteLater();
            }
            );
}
