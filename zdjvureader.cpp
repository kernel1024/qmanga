#include <QMutex>
#include <QBuffer>
#include <QDebug>

#include "zdjvureader.h"

#ifdef WITH_DJVU
static DDJVUAPI ddjvu_context_t  * djvuContext;
#endif

static QMutex indexerMutex;

ZDjVuReader::ZDjVuReader(QObject *parent, QString filename)
    : ZAbstractReader(parent, filename)
{
#ifdef WITH_DJVU
    document = nullptr;
#endif
    numPages = 0;
}

ZDjVuReader::~ZDjVuReader()
{
    closeFile();
}

bool ZDjVuReader::openFile()
{
    numPages = 0;

#ifdef WITH_DJVU
    if (opened || djvuContext==nullptr)
        return false;

    sortList.clear();

    QFileInfo fi(fileName);
    if (!fi.isFile() || !fi.isReadable()) {
        return false;
    }

    indexerMutex.lock();

    document = ddjvu_document_create_by_filename_utf8(djvuContext, fileName.toUtf8().constData(), 0);
    if (!document) {
        qWarning() << "Unable to create document context for " << fileName;
        indexerMutex.unlock();
        return false;
    }

    handle_ddjvu_messages(djvuContext, true);

    numPages = ddjvu_document_get_pagenum(document);

    for (int i=0;i<numPages;i++) {
        sortList << ZFileEntry(QString("%1").arg(i,6,10,QChar('0')),i);
    }

    qSort(sortList);

    indexerMutex.unlock();

    opened = true;
    return true;
#else
    return false;
#endif
}

void ZDjVuReader::closeFile()
{
#ifdef WITH_DJVU
    if (!opened)
        return;

    if (document)
        ddjvu_document_release(document);
    document = nullptr;
#endif

    opened = false;
    numPages = 0;
    sortList.clear();
}

QByteArray ZDjVuReader::loadPage(int num)
{
    QByteArray res;
    res.clear();
#ifdef WITH_DJVU
    if (!opened || document==nullptr || djvuContext==nullptr)
        return res;

    if (num<0 || num>=sortList.count()) return res;

    int idx = sortList.at(num).idx;

    ddjvu_page_t* page = ddjvu_page_create_by_pageno(document,idx);
    if (!page) return res;

    int resolution = ddjvu_page_get_resolution(page);

    while ( !ddjvu_page_decoding_done(page) )
        ;

    int w = ddjvu_page_get_width(page);
    int h = ddjvu_page_get_height(page);

    if (w < 1 || h < 1 ) {
        ddjvu_page_release(page);
        return res;
    }

    //ddjvu_page_set_rotation ( page, DDJVU_ROTATE_0 ); // need?
    qreal xdpi = zg->dpiX;
    qreal ydpi = zg->dpiY;
    if (zg->forceDPI>0.0) {
        xdpi = zg->forceDPI;
        ydpi = zg->forceDPI;
    }
    w = (int) ( w * xdpi / resolution );
    h = (int) ( h * ydpi / resolution );

    int row_stride = w * 4; // !!!!!!!!!!

    static uint masks[4] = { 0xff0000, 0xff00, 0xff, 0xff000000 };
    ddjvu_format_t* format = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 4, masks);
    if (!format)
    {
        ddjvu_page_release(page);
        return res;
    }

    ddjvu_format_set_row_order(format, 1);

    uchar* imageBuf = (uchar*)malloc(sizeof(uchar)* row_stride * h);

    QImage image(imageBuf, w, h, row_stride, QImage::Format_RGB32);

    ddjvu_rect_t pageRect;
    pageRect.x = 0; pageRect.y = 0;
    pageRect.w = w; pageRect.h = h;
    ddjvu_rect_t rendRect = pageRect;

    if (ddjvu_page_render(page, DDJVU_RENDER_COLOR,
                          &pageRect,
                          &rendRect,
                          format,
                          row_stride,
                          ( char* ) imageBuf ) )
    {
        QBuffer buf(&res);
        buf.open(QIODevice::WriteOnly);
        image.save(&buf,"BMP");
    }
    image = QImage();
    free(imageBuf);
    ddjvu_format_release(format);
    ddjvu_page_release(page);

#else
    Q_UNUSED(num)
#endif
    return res;
}

QString ZDjVuReader::getMagic()
{
    return QString("DJVU");
}

void ZDjVuReader::handle_ddjvu_messages ( ddjvu_context_t * ctx, int wait )
{
    const ddjvu_message_t *msg;
    if ( wait )
        ddjvu_message_wait ( ctx );
    while ( ( msg = ddjvu_message_peek ( ctx ) ) )
    {
        switch ( msg->m_any.tag )
        {
            case DDJVU_ERROR:      /*....*/ ; break;
            case DDJVU_INFO:       /*....*/ ; break;
            case DDJVU_NEWSTREAM:  /*....*/ ; break;
                //   ....
            default: break;
        }
        ddjvu_message_pop ( ctx );
    }
}


#ifdef WITH_DJVU

void initDjVuReader()
{
    djvuContext = ddjvu_context_create("qmanga");
    if (!djvuContext) {
        qCritical() << "Unable to create djvulibre context. DjVu support disabled.";
        djvuContext = nullptr;
    }
}

void freeDjVuReader()
{
    if (djvuContext)
        ddjvu_context_release(djvuContext);
    djvuContext = nullptr;
}

#else // WITH_DJVU

void initDjVuReader()
{
}

void freeDjVuReader()
{
}

#endif
