#include <QPageSize>
#include "zabstracttextdocument.h"
#include "zglobal.h"

ZAbstractTextDocument::ZAbstractTextDocument(QObject* parent)
    : QTextDocument(parent)
{
    m_pageMargin = (zF->global()->getTextDocMargin());

    documentLayout(); // initialize default QTextDocumentLayout

    // now setup page, font and other parameters with initialized layout
    setPageSize(QSizeF(ZAbstractTextDocument::defaultPageSizePixels()));
    setDocumentMargin(m_pageMargin);
    setDefaultFont(zF->global()->getTextDocFont());
}

ZAbstractTextDocument::~ZAbstractTextDocument() = default;

int ZAbstractTextDocument::maxContentHeight() const
{
    return static_cast<int>(pageSize().height()) - (2 * m_pageMargin);
}

int ZAbstractTextDocument::maxContentWidth() const
{
    return static_cast<int>(pageSize().width()) - (2 * m_pageMargin);
}

QSize ZAbstractTextDocument::defaultPageSizePixels()
{
    const qreal ppi = 72.0;
    const QPageSize paperPage = zF->global()->getTextDocPageSize();
    qreal xdpi = zF->global()->getDpiX();
    qreal ydpi = zF->global()->getDpiY();
    qreal forceDPI = zF->global()->getForceDPI();
    if (forceDPI>0.0) {
        xdpi = forceDPI;
        ydpi = forceDPI;
    }
    return QSize(qRound(paperPage.sizePoints().width() * xdpi / ppi),
                 qRound(paperPage.sizePoints().height() * ydpi / ppi));
}
