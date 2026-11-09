#include "jpreadertranslator.h"

#ifdef QT_DBUS_LIB
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

ZJPreaderTranslator::ZJPreaderTranslator(QObject *parent)
    : ZAbstractTranslator(parent)
{
    m_interface = new OrgKernel1024JpreaderAuxtranslatorInterface(
        QStringLiteral("org.kernel1024.jpreader"),
        QStringLiteral("/auxTranslator"), QDBusConnection::sessionBus(), this);

    connect(
        m_interface,&OrgKernel1024JpreaderAuxtranslatorInterface::translationRequestFinished,
        this,[this](qulonglong requestId, const QString &translation)
        {
            finishTranslation(requestId, translation);
        }
        );
}

bool ZJPreaderTranslator::isSame(ZAbstractTranslator *src) const
{
    if (!src)
        return false;

    auto* psrc = qobject_cast<ZJPreaderTranslator *>(src);

    if (!psrc)
        return false;

    return true;
}

bool ZJPreaderTranslator::isReady() const
{
    return (m_interface != nullptr) && m_interface->isValid();
}

void ZJPreaderTranslator::startTranslation(quint64 requestId, const QString &text,
                                           const QString &sourceLang, const QString &targetLang)
{
    if (!m_interface || !m_interface->isValid()) {
        finishTranslation(
            requestId,
            {},
            tr("jpreader D-Bus service is unavailable.")
            );
        return;
    }

    m_interface->startTranslationRequest(
        requestId,
        text,
        sourceLang,
        targetLang
        );
}

#endif
