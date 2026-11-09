#ifndef ZOLLAMATRANSLATOR_H
#define ZOLLAMATRANSLATOR_H

#include "abstracttranslator.h"
#include <QNetworkAccessManager>
#include <QUrl>

namespace ZDefaults {
const int ollamaConnectionTimeoutMS = 30000;
}

class ZOllamaTranslator final : public ZAbstractTranslator
{
    Q_OBJECT
public:
    explicit ZOllamaTranslator(const QUrl &baseUrl,
                               const QString &model,
                               QObject *parent = nullptr);
    bool isSame(ZAbstractTranslator *src) const override;
    bool isReady() const override;
    void setModel(const QString &model) { m_model = model; }
    void setBaseUrl(const QUrl &url) { m_baseUrl = url; }
    void setTimeoutMs(int ms) { m_timeoutMs = ms; }

protected:
    void startTranslation(quint64 requestId, const QString &text,
                          const QString &sourceLang, const QString &targetLang) override;

private:
    QNetworkAccessManager m_network;
    QUrl m_baseUrl;
    QString m_model;
    int m_timeoutMs { ZDefaults::ollamaConnectionTimeoutMS };
};

#endif
