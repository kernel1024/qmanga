/***************************************************************************
 *   Parts of this file from KDE Okular project.                           *
 *                                                                         *
 *   Copyright (C) 2008 by Ely Levy <elylevy@cs.huji.ac.il>                *
 *   Copyright (C) 2021 by kernelonline@gmail.com                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QTextCursor>
#include <QTextBlock>
#include <QRegularExpression>
#include <QDomDocument>
#include <QDomNodeList>
#include "zepubdocument.h"
#include "global.h"

#ifdef WITH_EPUB

namespace ZDefaults {
const int epubDebugLevel = 0; // none
}

ZEpubDocument::ZEpubDocument(QObject *parent, const QString &fileName)
    : ZAbstractTextDocument(parent)
{
    m_epub = epub_open(fileName.toUtf8().constData(), ZDefaults::epubDebugLevel);

    if (m_epub) {
        if (!epubConvert())
            epubClean();
    }
}

bool ZEpubDocument::isValid()
{
    return (m_epub != nullptr);
}

ZEpubDocument::~ZEpubDocument()
{
    epubClean();
}

bool ZEpubDocument::epubConvert()
{
    if (!isValid())
        return false;

    auto *cursor = new QTextCursor(this);

    struct eiterator *it = nullptr;

    // iterate over the book
    it = epub_get_iterator(m_epub, EITERATOR_SPINE, 0);

    // if the background color of the document is non-white it will be handled by QTextDocument::setHtml()
    bool firstPage = true;

    do {
        if (!epub_it_get_curr(it)) {
            continue;
        }

        const QString link = QString::fromUtf8(epub_it_get_curr_url(it));
        setCurrentSubDocument(link);
        QString htmlContent = QString::fromUtf8(epub_it_get_curr(it));

        // as QTextCharFormat::anchorNames() ignores sections, replace it with <p>
        htmlContent.replace(QRegularExpression(QSL("< *section")), QSL("<p"));
        htmlContent.replace(QRegularExpression(QSL("< */ *section")), QSL("</p"));

        // convert svg tags to img
        const int maxHeight = maxContentHeight();
        const int maxWidth = maxContentWidth();
        QDomDocument dom;
        if (dom.setContent(htmlContent)) {
            QDomNodeList svgs = dom.elementsByTagName(QSL("svg"));
            if (!svgs.isEmpty()) {
                QList<QDomNode> imgNodes;
                for (int i = 0; i < svgs.length(); ++i) {
                    QDomNodeList images = svgs.at(i).toElement().elementsByTagName(QSL("image"));
                    for (int j = 0; j < images.length(); ++j) {
                        const QString lnk = images.at(i).toElement().attribute(QSL("xlink:href"));
                        int ht = images.at(i).toElement().attribute(QSL("height")).toInt();
                        int wd = images.at(i).toElement().attribute(QSL("width")).toInt();
                        auto img = loadResource(QTextDocument::ImageResource, QUrl(lnk)).value<QImage>();
                        if (ht == 0)
                            ht = img.height();
                        if (wd == 0)
                            wd = img.width();
                        if (ht > maxHeight)
                            ht = maxHeight;
                        if (wd > maxWidth)
                            wd = maxWidth;

                        addResource(QTextDocument::ImageResource, QUrl(lnk), img);
                        QDomDocument newDoc;
                        newDoc.setContent(QSL("<img src=\"%1\" height=\"%2\" width=\"%3\" />").arg(lnk).arg(ht).arg(wd));
                        imgNodes.append(newDoc.documentElement());
                    }
                    for (const QDomNode &nd : qAsConst(imgNodes)) {
                        svgs.at(i).parentNode().replaceChild(nd, svgs.at(i));
                    }
                }
            }
            htmlContent = dom.toString();
        }

        QTextBlock before;
        if (firstPage) {
            setHtml(htmlContent);
            firstPage = false;
            before = begin();
        } else {
            before = cursor->block();
            cursor->insertHtml(htmlContent);
        }

        const int page = pageCount();

        // it will clear the previous format
        // useful when the last line had a bullet
        cursor->insertBlock(QTextBlockFormat());

        while (pageCount() == page)
            cursor->insertText(QSL("\n"));

    } while (epub_it_get_next(it) != nullptr);

    epub_free_iterator(it);

    delete cursor;
    return true;
}

void ZEpubDocument::epubClean()
{
    if (m_epub)
        epub_close(m_epub);
    m_epub = nullptr;

    epub_cleanup();
}

void ZEpubDocument::setCurrentSubDocument(const QString &doc)
{
    m_currentSubDocument.clear();
    int index = doc.indexOf('/');
    if (index > 0) {
        m_currentSubDocument = QUrl::fromLocalFile(doc.left(index + 1));
    }
}

QString ZEpubDocument::checkCSS(const QString &c)
{
    QString css = c;
    // remove paragraph line-heights
    css.remove(QRegularExpression(QSL("line-height\\s*:\\s*[\\w\\.]*;")));

    // HACK transform em and rem notation to px, because QTextDocument doesn't support
    // em and rem.
    const QStringList cssArray = css.split(QRegularExpression(QSL("\\s+")));
    QStringList cssArrayReplaced;
    std::size_t cssArrayCount = cssArray.count();
    std::size_t i = 0;
    const QRegularExpression re(QSL("(([0-9]+)(\\.[0-9]+)?)r?em(.*)"));
    while (i < cssArrayCount) {
        auto item = cssArray[i];
        QRegularExpressionMatch match = re.match(item);
        if (match.hasMatch()) {
            double em = match.captured(1).toDouble();
            double px = em * defaultFont().pointSize();
            cssArrayReplaced.append(QSL("%1px%2").arg(px).arg(match.captured(4)));
        } else {
            cssArrayReplaced.append(item);
        }
        i++;
    }
    return cssArrayReplaced.join(QSL(" "));
}

QVariant ZEpubDocument::loadResource(int type, const QUrl &name)
{
    QString fileInPath = m_currentSubDocument.resolved(name).path();

    // Get the data from the epub file
    char *data = nullptr;
    int size = epub_get_data(m_epub, fileInPath.toUtf8().constData(), &data);

    QVariant resource;

    if (data) {
        switch (type) {
            case QTextDocument::ImageResource: {
                QImage img = QImage::fromData(reinterpret_cast<unsigned char *>(data), size);
                const int maxHeight = maxContentHeight();
                const int maxWidth = maxContentWidth();
                if (img.height() > maxHeight)
                    img = img.scaledToHeight(maxHeight, Qt::SmoothTransformation);
                if (img.width() > maxWidth)
                    img = img.scaledToWidth(maxWidth, Qt::SmoothTransformation);
                resource.setValue(img);
                break;
            }
            case QTextDocument::StyleSheetResource: {
                QString css = QString::fromUtf8(data);
                resource.setValue(checkCSS(css));
                break;
            }
            default:
                resource.setValue(QString::fromUtf8(data));
                break;
        }

        free(data); // NOLINT
    }

    // add to cache
    addResource(type, name, resource);

    return resource;
}

#endif // WITH_EPUB
