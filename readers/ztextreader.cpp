#include <QPainter>
#include <QAbstractTextDocumentLayout>
#include <QPageSize>
#include "ztextreader.h"
#include "zglobal.h"
#include "ztextdocumentcontroller.h"

#include "textdoc/ztxtdocument.h"
#include "textdoc/zepubdocument.h"
#include "textdoc/zfb2document.h"

ZTextReader::ZTextReader(QObject *parent, const QString &filename)
    : ZAbstractReader(parent,filename)
{
}

bool ZTextReader::preloadFile(const QString &filename, bool preserveDocument)
{
    static QMutex mutex;
    QMutexLocker locker(&mutex); // parser mutex

    static const QStringList fb2Suffixes({ QSL("fb2"), QSL("fb"), QSL("fb2.zip"), QSL("fb.zip") });

    if (zF->global()->txtController()->contains(filename))
        return true;

    QFileInfo fi(filename);
    ZAbstractTextDocument* doc = nullptr;

    if (fi.suffix().compare(QSL("txt"),Qt::CaseInsensitive) == 0) {
        doc = new ZTxtDocument(nullptr,filename);
    } else if (fb2Suffixes.contains(fi.completeSuffix(),Qt::CaseInsensitive)) {
        doc = new ZFB2Document(nullptr,filename);
#ifdef WITH_EPUB
    } else if (fi.suffix().compare(QSL("epub"),Qt::CaseInsensitive) == 0) {
        doc = new ZEpubDocument(nullptr,filename);
#endif
    }

    if (doc == nullptr)
        return false;

    if (!doc->isValid()) {
        delete doc;
        return false;
    }

    if (preserveDocument) {
        zF->global()->txtController()->addDocument(filename,doc);
    } else {
        delete doc;
    }

    return true;
}

ZTextReader::~ZTextReader() = default;

QFont ZTextReader::getDefaultFont() const
{
    return m_defaultFont;
}

bool ZTextReader::openFile()
{
    if (isOpened())
        return false;

    m_doc = zF->global()->txtController()->getDocument(getFileName());
    if (m_doc == nullptr)
        return false;

    m_doc->setPageSize(QSizeF(ZAbstractTextDocument::defaultPageSizePixels()));

    const int pageCounterWidth = 6;
    const int pageCounterBase = 10;
    for (int i=0; i<m_doc->pageCount(); i++) {
        addSortEntry(QSL("%1").arg(i,pageCounterWidth,pageCounterBase,QChar('0')),i);
    }

    performListSort();
    setOpenFileSuccess();
    return true;
}

void ZTextReader::closeFile()
{
    if (!isOpened())
        return;

    if (m_doc)
        delete m_doc;

    ZAbstractReader::closeFile();
}

QByteArray ZTextReader::loadPage(int num)
{
    Q_UNUSED(num)

    return QByteArray();
}

QImage ZTextReader::loadPageImage(int num)
{
    if (m_doc.isNull())
        return QImage();

    QSize size = m_doc->pageSize().toSize();
    QImage res(size.width(), size.height(), QImage::Format_ARGB32);
    res.fill(zF->global()->getTextDocBkColor());

    int idx = getSortEntryIdx(num);
    if (idx<0)
        return res;

    QPainter p;
    p.begin(&res);

    QRect rect;
    rect = QRect(0, idx * size.height(), size.width(), size.height());
    p.translate(QPoint(0, idx * size.height() * -1));
    p.setClipRect(rect);

    QAbstractTextDocumentLayout::PaintContext context;
    context.palette.setColor(QPalette::Text, Qt::black);
    context.clip = rect;
    m_doc->documentLayout()->draw(&p, context);

    p.end();

    return res;
}

bool ZTextReader::isPageDataSupported()
{
    return false;
}

QString ZTextReader::getMagic()
{
    return QSL("TEXT");
}
