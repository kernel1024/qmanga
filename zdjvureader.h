#ifndef ZDJVUREADER_H
#define ZDJVUREADER_H

#include <QObject>
#include <QMutex>

#ifdef WITH_DJVU
#include <libdjvu/ddjvuapi.h>
#include <libdjvu/miniexp.h>
#endif

#include "zabstractreader.h"

class ZDjVuReader : public ZAbstractReader
{
    Q_OBJECT
public:
    ZDjVuReader(QObject *parent, const QString &filename);
    ~ZDjVuReader() override;
    bool openFile() override;
    void closeFile() override;
    QByteArray loadPage(int num) override;
    QImage loadPageImage(int num) override;
    QString getMagic() override;

private:
    Q_DISABLE_COPY(ZDjVuReader)

};

#ifdef WITH_DJVU
class ZDjVuDocument {
public:
    ddjvu_document_t * document { nullptr };
    QString filename;
    int pageNum { 0 };
    int ref { 0 };
    ZDjVuDocument() = default;
    ~ZDjVuDocument() = default;
    ZDjVuDocument(const ZDjVuDocument& other);
    explicit ZDjVuDocument(const QString& aFilename);
    ZDjVuDocument(ddjvu_document_t * aDocument, const QString& aFilename,  int aPageNum);
    ZDjVuDocument &operator=(const ZDjVuDocument& other) = default;
    bool operator==(const ZDjVuDocument& ref) const;
    bool operator!=(const ZDjVuDocument& ref) const;
};
#endif

class ZDjVuController : public QObject
{
    Q_OBJECT
public:
    explicit ZDjVuController(QObject* parent = nullptr);
    ~ZDjVuController() override;
    static ZDjVuController *instance();
    void initDjVuReader();
    void freeDjVuReader();
    bool loadDjVu(const QString& filename, int &numPages);
    void closeDjVu(const QString& filename);
#ifdef WITH_DJVU
    ddjvu_document_t* getDocument(const QString &filename);
#endif

private:
    static ZDjVuController* m_instance;
    QMutex m_docMutex;
#ifdef WITH_DJVU
    ddjvu_context_t  * m_djvuContext { nullptr };
    QList<ZDjVuDocument> m_documents;

    void handle_ddjvu_messages(ddjvu_context_t *ctx, bool wait);
#endif

    Q_DISABLE_COPY(ZDjVuController)

    friend class ZDjVuReader;
};


#endif // ZDJVUREADER_H
