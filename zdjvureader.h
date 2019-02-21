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

private:
    int numPages;

public:
    explicit ZDjVuReader(QObject *parent, const QString &filename);
    ~ZDjVuReader();
    bool openFile();
    void closeFile();
    QByteArray loadPage(int num);
    QString getMagic();

};

#ifdef WITH_DJVU
class ZDjVuDocument {
public:
    DDJVUAPI ddjvu_document_t * document;
    QString filename;
    int pageNum;
    int ref;
    ZDjVuDocument();
    ZDjVuDocument(const ZDjVuDocument& other);
    ZDjVuDocument(const QString& aFilename);
    ZDjVuDocument(DDJVUAPI ddjvu_document_t * aDocument, const QString& aFilename,  int aPageNum);
    ZDjVuDocument &operator=(const ZDjVuDocument& other) = default;
    bool operator==(const ZDjVuDocument& ref) const;
    bool operator!=(const ZDjVuDocument& ref) const;
};
#endif

class ZDjVuController : public QObject
{
    Q_OBJECT

private:
    static ZDjVuController* m_instance;
    QMutex docMutex;
#ifdef WITH_DJVU
    DDJVUAPI ddjvu_context_t  * djvuContext;
    QList<ZDjVuDocument> documents;

    void handle_ddjvu_messages(ddjvu_context_t *ctx, int wait);
#endif

public:
    static ZDjVuController *instance();
    void initDjVuReader();
    void freeDjVuReader();
    bool loadDjVu(const QString& filename, int &numPages);
    void closeDjVu(const QString& filename);
#ifdef WITH_DJVU
    DDJVUAPI ddjvu_document_t* getDocument(const QString &filename);
#endif

    friend class ZDjVuReader;

};


#endif // ZDJVUREADER_H
