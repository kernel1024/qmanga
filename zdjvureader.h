#ifndef ZDJVUREADER_H
#define ZDJVUREADER_H

#include <QObject>

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

#ifdef WITH_DJVU
    DDJVUAPI ddjvu_document_t * document;

    void handle_ddjvu_messages(ddjvu_context_t *ctx, int wait);
#endif

public:
    explicit ZDjVuReader(QObject *parent, QString filename);
    ~ZDjVuReader();
    bool openFile();
    void closeFile();
    QByteArray loadPage(int num);
    QString getMagic();

};

void initDjVuReader();
void freeDjVuReader();

#endif // ZDJVUREADER_H
