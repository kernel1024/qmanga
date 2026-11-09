#ifndef ZJPREADERTRANSLATOR_H
#define ZJPREADERTRANSLATOR_H

#include "abstracttranslator.h"

#ifdef QT_DBUS_LIB
#include "auxtranslator_interface.h"
#include <QDBusConnection>

class ZJPreaderTranslator final : public ZAbstractTranslator
{
    Q_OBJECT
public:
    explicit ZJPreaderTranslator(QObject *parent = nullptr);
    bool isSame(ZAbstractTranslator *src) const override;
    bool isReady() const override;

protected:
    void startTranslation(quint64 requestId, const QString &text,
                          const QString &sourceLang, const QString &targetLang) override;

private:
    OrgKernel1024JpreaderAuxtranslatorInterface *m_interface = nullptr;
};
#endif

#endif
