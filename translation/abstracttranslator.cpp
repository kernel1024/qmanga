#include <QMetaObject>
#include "../zglobal.h"

#include "abstracttranslator.h"
#include "jpreadertranslator.h"
#include "ollamatranslator.h"

ZAbstractTranslator::ZAbstractTranslator(QObject *parent) : QObject(parent)
{
}

void ZAbstractTranslator::translate(
    const QString &text,
    const QString &sourceLang,
    const QString &targetLang,
    QObject *context,
    TranslationCallback callback)
{
    if (!context || !callback)
        return;

    const quint64 requestId = ++m_nextRequestId;

    m_pending.insert(
        requestId,
        {context, std::move(callback)}
        );

    connect(context, &QObject::destroyed, this,
            [this, requestId]()
            {
                m_pending.remove(requestId);
            }
            );

    startTranslation(requestId, text, sourceLang, targetLang);
}

void ZAbstractTranslator::finishTranslation(quint64 requestId, const QString &translation, const QString &error)
{
    const auto it = m_pending.find(requestId);

    if (it == m_pending.end())
        return;

    PendingRequest pending = std::move(it.value());

    m_pending.erase(it);

    if (!pending.context)
        return;

    QMetaObject::invokeMethod(pending.context,
                              [callback = std::move(pending.callback), translation, error]() mutable
                              {
                                  callback(translation, error);
                              },
                              Qt::QueuedConnection);
}

ZAbstractTranslator* ZAbstractTranslator::factory(QObject *parent)
{
#ifdef QT_DBUS_LIB
    if (zF->global()->getTranslationEngine() == Z::tranJPReader)
        return new ZJPreaderTranslator(parent);
#endif
    if (zF->global()->getTranslationEngine() == Z::tranOllama)
        return new ZOllamaTranslator(zF->global()->getOllamaBaseUrl(),
                                     zF->global()->getOllamaModel(),
                                     parent);

    return nullptr;
}
