#ifndef ZABSTRACTTRANSLATOR_H
#define ZABSTRACTTRANSLATOR_H

#include <QObject>
#include <QHash>
#include <QPointer>
#include <QString>
#include <QtGlobal>
#include <functional>

// Asynchronous translation transport, independent of OCR and platform.
class ZAbstractTranslator : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ZAbstractTranslator)
public:
    using TranslationCallback =
        std::function<void(
            const QString &translation,
            const QString &error
            )>;

    explicit ZAbstractTranslator(QObject *parent = nullptr);
    ~ZAbstractTranslator() override = default;

    virtual bool isSame(ZAbstractTranslator *src) const = 0;
    virtual bool isReady() const { return false; }
    virtual void translate(const QString &text,
                           const QString &sourceLang,
                           const QString &targetLang,
                           QObject *context,
                           TranslationCallback callback
                           );
    static ZAbstractTranslator* factory(QObject *parent);

protected:
    virtual void startTranslation(
        quint64 requestId,
        const QString &text,
        const QString &sourceLang,
        const QString &targetLang
        ) = 0;

    void finishTranslation(
        quint64 requestId,
        const QString &translation,
        const QString &error = {}
        );

private:
    struct PendingRequest
    {
        QPointer<QObject> context;
        TranslationCallback callback;
    };

    QHash<quint64, PendingRequest> m_pending;
    quint64 m_nextRequestId = 0;
};

#endif
