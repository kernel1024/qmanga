#include <QMutex>
#include <QBuffer>
#include <QDebug>

#include "zdjvureader.h"

static QMutex indexerMutex;

ZDjVuReader::ZDjVuReader(QObject *parent, const QString &filename)
    : ZAbstractReader(parent, filename),
      numPages(0)
{
}

ZDjVuReader::~ZDjVuReader()
{
    closeFile();
}

bool ZDjVuReader::openFile()
{
    numPages = 0;

#ifdef WITH_DJVU
    if (opened)
        return false;

    sortList.clear();

    if (!ZDjVuController::instance()->loadDjVu(fileName, numPages)) {
        qWarning() << tr("Unable to load file ") << fileName;
        return false;
    }

    for (int i=0;i<numPages;i++) {
        sortList << ZFileEntry(QString("%1").arg(i,6,10,QChar('0')),i);
    }

    qSort(sortList);

    opened = true;
    return true;
#else
    return false;
#endif
}

void ZDjVuReader::closeFile()
{
    if (!opened)
        return;
#ifdef WITH_DJVU
    ZDjVuController::instance()->closeDjVu(fileName);
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
    ddjvu_document_t* document = ZDjVuController::instance()->getDocument(fileName);
    if (!opened || document==nullptr) {
        qWarning() << "Uninitialized context for page " << num;
        return res;
    }

    if (num<0 || num>=sortList.count()) {
        qWarning() << "Incorrect page number " << num;
        return res;
    }

    int idx = sortList.at(num).idx;

    ddjvu_page_t* page = ddjvu_page_create_by_pageno(document,idx);
    if (!page) {
        qWarning() << "Unable to create page " << idx;
        return res;
    }

    int resolution = ddjvu_page_get_resolution(page);

    while ( !ddjvu_page_decoding_done(page) )
        ;

    int w = ddjvu_page_get_width(page);
    int h = ddjvu_page_get_height(page);

    if (w < 1 || h < 1 ) {
        qWarning() << "Null-sized page " << idx;
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
    w = static_cast<int>( w * xdpi / resolution );
    h = static_cast<int>( h * ydpi / resolution );

    ulong row_stride = static_cast<uint>(w) * 4; // !!!!!!!!!!

    static uint masks[4] = { 0xff0000, 0xff00, 0xff, 0xff000000 };
    ddjvu_format_t* format = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 4, masks);
    if (!format)
    {
        qWarning() << "Unable to create format for page " << idx;
        ddjvu_page_release(page);
        return res;
    }

    ddjvu_format_set_row_order(format, 1);

    ulong bufSize = sizeof(uchar)* row_stride * static_cast<ulong>(h);
    auto imageBuf = static_cast<uchar *>(malloc(bufSize));

    QImage image(imageBuf, w, h, static_cast<int>(row_stride), QImage::Format_RGB32);

    ddjvu_rect_t pageRect;
    pageRect.x = 0; pageRect.y = 0;
    pageRect.w = static_cast<uint>(w);
    pageRect.h = static_cast<uint>(h);
    ddjvu_rect_t rendRect = pageRect;

    if (ddjvu_page_render(page, DDJVU_RENDER_COLOR,
                          &pageRect,
                          &rendRect,
                          format,
                          static_cast<uint>(row_stride),
                          reinterpret_cast<char *>(imageBuf)))
    {
        QBuffer buf(&res);
        buf.open(QIODevice::WriteOnly);
        image.save(&buf,"BMP");
        buf.close();
    } else
        qWarning() << "Unable to render page " << idx;

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

ZDjVuController* ZDjVuController::m_instance = nullptr;

ZDjVuController *ZDjVuController::instance()
{
    if (!m_instance)
        m_instance = new ZDjVuController();
    return m_instance;
}

void ZDjVuController::initDjVuReader()
{
#ifdef WITH_DJVU
    djvuContext = ddjvu_context_create("qmanga");
    if (!djvuContext) {
        qCritical() << "Unable to create djvulibre context. DjVu support disabled.";
        djvuContext = nullptr;
    }
    documents.clear();
#endif
}

void ZDjVuController::freeDjVuReader()
{
#ifdef WITH_DJVU
    while (!documents.isEmpty()) {
        ZDjVuDocument doc = documents.takeFirst();
        if (doc.document)
            ddjvu_document_release(doc.document);
    }

    if (djvuContext)
        ddjvu_context_release(djvuContext);
    djvuContext = nullptr;
#endif
}

static QMutex docMutex;

bool ZDjVuController::loadDjVu(const QString &filename, int &numPages)
{
#ifdef WITH_DJVU
    if (djvuContext==nullptr) {
        qWarning() << "No context for " << filename;
        return false;
    }

    docMutex.lock();

    int dIdx = documents.indexOf(ZDjVuDocument(filename));
    if (dIdx >= 0) {
        numPages = documents.at(dIdx).pageNum;
        documents[dIdx].ref++;
        docMutex.unlock();
        return true;
    }

    QFileInfo fi(filename);
    if (!fi.isFile() || !fi.isReadable()) {
        qWarning() << "File is not readable " << filename;
        docMutex.unlock();
        return false;
    }


    ddjvu_document_t* document = ddjvu_document_create_by_filename_utf8(
                                              djvuContext, filename.toUtf8().constData(), 0);
    if (!document) {
        qWarning() << "Unable to create document context for " << filename;
        docMutex.unlock();
        return false;
    }

    handle_ddjvu_messages(djvuContext, true);

    numPages = ddjvu_document_get_pagenum(document);

    documents.append(ZDjVuDocument(document, filename, numPages));

    docMutex.unlock();

    return true;
#else
    Q_UNUSED(filename)
    Q_UNUSED(numPages)
    return false;
#endif
}

void ZDjVuController::closeDjVu(const QString &filename)
{
#ifdef WITH_DJVU
    docMutex.lock();
    int dIdx = documents.indexOf(ZDjVuDocument(filename));
    if (dIdx >= 0) {
        documents[dIdx].ref--;
        if (documents.at(dIdx).ref==0) {
            ZDjVuDocument doc = documents.takeAt(dIdx);
            ddjvu_document_release(doc.document);
        }
    }
    docMutex.unlock();
#else
    Q_UNUSED(filename)
#endif
}

#ifdef WITH_DJVU
ddjvu_document_t *ZDjVuController::getDocument(const QString& filename)
{
    docMutex.lock();
    ddjvu_document_t* res = nullptr;
    int dIdx = documents.indexOf(ZDjVuDocument(filename));
    if (dIdx >= 0)
        res = documents.at(dIdx).document;
    else
        qWarning() << "Unable to find opened document " << filename;
    docMutex.unlock();
    return res;
}

void ZDjVuController::handle_ddjvu_messages ( ddjvu_context_t * ctx, int wait )
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

ZDjVuDocument::ZDjVuDocument()
    : document(nullptr),
      pageNum(0),
      ref(0)
{
    filename.clear();
}

ZDjVuDocument::ZDjVuDocument(const ZDjVuDocument &other)
{
    document = other.document;
    filename = other.filename;
    pageNum = other.pageNum;
    ref = other.ref;
}

ZDjVuDocument::ZDjVuDocument(const QString &aFilename)
    : document(nullptr),
      pageNum(0),
      ref(0)
{
    filename = aFilename;
}

ZDjVuDocument::ZDjVuDocument(ddjvu_document_t *aDocument, const QString &aFilename, int aPageNum)
{
    document = aDocument;
    filename = aFilename;
    pageNum = aPageNum;
    ref = 1;
}

bool ZDjVuDocument::operator==(const ZDjVuDocument &ref) const
{
    return (filename==ref.filename);
}

bool ZDjVuDocument::operator!=(const ZDjVuDocument &ref) const
{
    return (filename!=ref.filename);
}
#endif
