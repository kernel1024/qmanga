#include <algorithm>
#include <QBuffer>
#include <QMutexLocker>
#include <QDebug>

#include "zdjvureader.h"

ZDjVuReader::ZDjVuReader(QObject *parent, const QString &filename)
    : ZAbstractReader(parent, filename)
{
}

ZDjVuReader::~ZDjVuReader()
{
    closeFile();
}

bool ZDjVuReader::openFile()
{
#ifdef WITH_DJVU
    if (isOpened())
        return false;

    int numPages;
    if (!ZDjVuController::instance()->loadDjVu(getFileName(), numPages)) {
        qWarning() << tr("Unable to load file ") << getFileName();
        return false;
    }

    constexpr int pageCounterWidth = 6;
    constexpr int pageCounterBase = 10;
    for (int i=0;i<numPages;i++) {
        addSortEntry(ZFileEntry(QSL("%1")
                                .arg(i,pageCounterWidth,pageCounterBase,QChar('0')),i));
    }

    performListSort();
    setOpenFileSuccess();
    return true;
#else
    return false;
#endif
}

void ZDjVuReader::closeFile()
{
    if (!isOpened())
        return;
#ifdef WITH_DJVU
    ZDjVuController::instance()->closeDjVu(getFileName());
#endif

    ZAbstractReader::closeFile();
}

QImage ZDjVuReader::loadPageImage(int num)
{
#ifdef WITH_DJVU
    ddjvu_document_t* document = ZDjVuController::instance()->getDocument(getFileName());
    if (!isOpened() || document==nullptr) {
        qWarning() << "Uninitialized context for page " << num;
        return QImage();
    }

    int idx = getSortEntryIdx(num);
    if (idx<0) {
        qCritical() << "Incorrect page number " << num;
        return QImage();
    }

    ddjvu_page_t* page = ddjvu_page_create_by_pageno(document,idx);
    if (!page) {
        qWarning() << "Unable to create page " << idx;
        return QImage();
    }

    int resolution = ddjvu_page_get_resolution(page);

    while ( !ddjvu_page_decoding_done(page) )
        ;

    int w = ddjvu_page_get_width(page);
    int h = ddjvu_page_get_height(page);

    if (w < 1 || h < 1 ) {
        qWarning() << "Null-sized page " << idx;
        ddjvu_page_release(page);
        return QImage();
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

    const uint redMask = 0xff0000;
    const uint greenMask = 0xff00;
    const uint blueMask = 0xff;
    const uint alphaMask = 0xff000000;
    uint masks[4] = { redMask, greenMask, blueMask, alphaMask };
    ddjvu_format_t* format = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 4, masks);
    if (!format)
    {
        qWarning() << "Unable to create format for page " << idx;
        ddjvu_page_release(page);
        return QImage();
    }

    ddjvu_format_set_row_order(format, 1);

    int row_stride = w * 4; // !!!!!!!!!!

    QImage image(w, h, QImage::Format_RGB32);

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
                          reinterpret_cast<char *>(image.bits()))==0)
        qWarning() << "Unable to render page " << idx;

    ddjvu_format_release(format);
    ddjvu_page_release(page);

    if (!image.isNull())
        return image;

#else
    Q_UNUSED(num)
#endif
    return QImage();
}

QByteArray ZDjVuReader::loadPage(int num)
{
    QByteArray res;

    QImage img = loadPageImage(num);
    if (img.isNull()) return res;

    QBuffer buf(&res);
    img.save(&buf,"BMP");

    return res;
}

QString ZDjVuReader::getMagic()
{
    return QSL("DJVU");
}

ZDjVuController* ZDjVuController::m_instance = nullptr;

ZDjVuController::ZDjVuController(QObject *parent)
    : QObject(parent)
{

}

ZDjVuController::~ZDjVuController()
{
    freeDjVuReader();
}

ZDjVuController *ZDjVuController::instance()
{
    if (!m_instance)
        m_instance = new ZDjVuController();
    return m_instance;
}

void ZDjVuController::initDjVuReader()
{
#ifdef WITH_DJVU
    m_djvuContext = ddjvu_context_create("qmanga");
    if (!m_djvuContext) {
        qCritical() << "Unable to create djvulibre context. DjVu support disabled.";
        m_djvuContext = nullptr;
    }
    m_documents.clear();
#endif
}

void ZDjVuController::freeDjVuReader()
{
#ifdef WITH_DJVU
    while (!m_documents.isEmpty()) {
        ZDjVuDocument doc = m_documents.takeFirst();
        if (doc.document)
            ddjvu_document_release(doc.document);
    }

    if (m_djvuContext)
        ddjvu_context_release(m_djvuContext);
    m_djvuContext = nullptr;
#endif
}

bool ZDjVuController::loadDjVu(const QString &filename, int &numPages)
{
    QMutexLocker locker(&m_docMutex);

#ifdef WITH_DJVU
    if (m_djvuContext==nullptr) {
        qWarning() << "No context for " << filename;
        return false;
    }

    int dIdx = m_documents.indexOf(ZDjVuDocument(filename));
    if (dIdx >= 0) {
        numPages = m_documents.at(dIdx).pageNum;
        m_documents[dIdx].ref++;
        return true;
    }

    QFileInfo fi(filename);
    if (!fi.isFile() || !fi.isReadable()) {
        qWarning() << "File is not readable " << filename;
        return false;
    }


    ddjvu_document_t* document = ddjvu_document_create_by_filename_utf8(
                                              m_djvuContext, filename.toUtf8().constData(), 0);
    if (!document) {
        qWarning() << "Unable to create document context for " << filename;
        return false;
    }

    handle_ddjvu_messages(m_djvuContext, true);

    numPages = ddjvu_document_get_pagenum(document);

    m_documents.append(ZDjVuDocument(document, filename, numPages));

    return true;
#else
    Q_UNUSED(filename)
    Q_UNUSED(numPages)
    return false;
#endif
}

void ZDjVuController::closeDjVu(const QString &filename)
{
    QMutexLocker locker(&m_docMutex);
#ifdef WITH_DJVU
    int dIdx = m_documents.indexOf(ZDjVuDocument(filename));
    if (dIdx >= 0) {
        m_documents[dIdx].ref--;
        if (m_documents.at(dIdx).ref==0) {
            ZDjVuDocument doc = m_documents.takeAt(dIdx);
            ddjvu_document_release(doc.document);
        }
    }
#else
    Q_UNUSED(filename)
#endif
}

#ifdef WITH_DJVU
ddjvu_document_t *ZDjVuController::getDocument(const QString& filename)
{
    QMutexLocker locker(&m_docMutex);

    ddjvu_document_t* res = nullptr;
    int dIdx = m_documents.indexOf(ZDjVuDocument(filename));
    if (dIdx >= 0) {
        res = m_documents.at(dIdx).document;
    } else {
        qWarning() << "Unable to find opened document " << filename;
    }
    return res;
}

void ZDjVuController::handle_ddjvu_messages ( ddjvu_context_t * ctx, bool wait )
{
    const ddjvu_message_t *msg;
    if ( wait )
        ddjvu_message_wait ( ctx );
    while ( ( msg = ddjvu_message_peek ( ctx ) ) )
    {
        switch ( msg->m_any.tag )
        {
            case DDJVU_ERROR: qCritical() << "DJVU error: " << msg->m_error.message; break;
            case DDJVU_INFO: qInfo() << "DJVU info: " << msg->m_info.message; break;
            default: break;
        }
        ddjvu_message_pop ( ctx );
    }
}

ZDjVuDocument::ZDjVuDocument(const ZDjVuDocument &other)
{
    document = other.document;
    filename = other.filename;
    pageNum = other.pageNum;
    ref = other.ref;
}

ZDjVuDocument::ZDjVuDocument(const QString &aFilename)
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
