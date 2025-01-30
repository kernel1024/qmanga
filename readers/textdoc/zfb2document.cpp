/***************************************************************************
 *   Parts of this file from KDE Okular project.                           *
 *                                                                         *
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *   Copyright (C) 2021 by kernelonline@gmail.com                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QScopeGuard>
#include <QDebug>

#include <zip.h>

#include "zfb2document.h"
#include "global.h"

namespace ZDefaults {
const int fb2TitleBorder = 2;
const int fb2TitlePadding = 8;
const int fb2SubTitleMargin = 16;
const int fb2TitleFontSizeTitle = 22;
const int fb2TitleFontSizeAuthor = 10;
const int fb2TitleFontSizeAnnotation = 10;
const int fb2ParagraphIndent = 10;
const int fb2AuthorParagraphIndent = 10;
const int fb2StanzaIndent = 50;
const auto fb2TitleBackground = Qt::lightGray;

}

class ZFB2TitleInfo
{
public:
    QStringList mGenres;
    QString mAuthor;
    QString mTitle;
    QString mAnnotation;
    QStringList mKeywords;
    QDate mDate;
    QDomElement mCoverPage;
    QString mLanguage;
};

ZFB2Document::ZFB2Document(QObject* parent, const QString &fileName)
    : ZAbstractTextDocument(parent)
{
    const QString mime = ZGenericFuncs::detectMIME(fileName);
    const QStringList validSuffixes({ QSL("fb"), QSL("fb2") });
    QByteArray fb2data;

    if (mime.contains(QSL("application/zip"),Qt::CaseInsensitive)) {
        zip_t* zFile = zip_open(fileName.toUtf8().constData(),ZIP_RDONLY,nullptr);
        if (zFile) {
            qint64 count = zip_get_num_entries(zFile,0);
            for (qint64 idx = 0; idx < count; idx++) {
                zip_stat_t stat;
                if (zip_stat_index(zFile,static_cast<uint>(idx),
                                   ZIP_STAT_SIZE | ZIP_STAT_NAME,&stat)<0) {
                    qDebug() << "Unable to get file stat for index " << idx;
                    continue;
                }
                QString fname = QString::fromUtf8(stat.name);
                if (fname.endsWith(u'/') || fname.endsWith(u'\\')) continue;

                QFileInfo fi(fname);
                if (validSuffixes.contains(fi.suffix(),Qt::CaseInsensitive)) {
                    fb2data.resize(stat.size);
                    zip_file_t* file = zip_fopen_index(zFile,static_cast<uint>(idx),0);
                    if (file != nullptr) {
                        qint64 sz = zip_fread(file,fb2data.data(),stat.size);
                        zip_fclose(file);

                        if (sz > 0) {
                            if (static_cast<uint>(sz) < stat.size)
                                fb2data.truncate(static_cast<int>(sz));
                            break;
                        }
                    }
                }
            }
            zip_close(zFile);
        }
    }

    if (fb2data.isEmpty()) { // No zip file
        QFile f(fileName);
        if (f.open(QIODevice::ReadOnly)) {
            fb2data = f.readAll();
            f.close();
        }
    }

    if (!fb2data.isEmpty()) {
        m_loadedOk = fb2Convert(fb2data);
        if (!m_loadedOk)
            clear();
    }
}

ZFB2Document::~ZFB2Document() = default;

bool ZFB2Document::isValid()
{
    return m_loadedOk;
}

bool ZFB2Document::fb2Convert(const QByteArray &data)
{
    mCursor = new QTextCursor(this);

    auto cleanup = qScopeGuard([this]{
        delete mCursor;
    });

    mSectionCounter = 0;

    QDomDocument document;
    auto parseResult = document.setContent(ZGenericFuncs::detectDecodeToUnicode(data));
    if (!parseResult) {
        qCritical() << "Invalid XML FB2 structure, unable to load XML. " << parseResult.errorMessage;
        return false;
    }

    /**
     * Parse the content of the document
     */
    const QDomElement documentElement = document.documentElement();

    if (documentElement.tagName() != QSL("FictionBook")) {
        qCritical() << tr("Document is not a valid FictionBook");
        return false;
    }

    /**
     * First we read all images, so we can calculate the size later.
     */
    QDomElement element = documentElement.firstChildElement();
    while (!element.isNull()) {
        if (element.tagName() == QSL("binary")) {
            if (!convertBinary(element)) {
                return false;
            }
        }

        element = element.nextSiblingElement();
    }

    /**
     * Read the rest...
     */
    element = documentElement.firstChildElement();
    while (!element.isNull()) {
        if (element.tagName() == QSL("description")) {
            if (!convertDescription(element)) {
                return false;
            }
        } else if (element.tagName() == QSL("body")) {
            if (mTitleInfo && !mTitleInfo->mCoverPage.isNull()) {
                convertCover(mTitleInfo->mCoverPage);
                mCursor->insertBlock();
            }

            QTextFrame *topFrame = mCursor->currentFrame();

            QTextFrameFormat frameFormat;
            frameFormat.setBorder(ZDefaults::fb2TitleBorder);
            frameFormat.setPadding(ZDefaults::fb2TitlePadding);
            frameFormat.setBackground(ZDefaults::fb2TitleBackground);

            if (mTitleInfo && !mTitleInfo->mTitle.isEmpty()) {
                mCursor->insertFrame(frameFormat);

                QTextCharFormat charFormat;
                charFormat.setFontPointSize(ZDefaults::fb2TitleFontSizeTitle);
                charFormat.setFontWeight(QFont::Bold);
                mCursor->insertText(mTitleInfo->mTitle, charFormat);

                mCursor->setPosition(topFrame->lastPosition());
            }

            if (mTitleInfo && !mTitleInfo->mAuthor.isEmpty()) {
                frameFormat.setBorder(1);
                mCursor->insertFrame(frameFormat);

                QTextCharFormat charFormat;
                charFormat.setFontPointSize(ZDefaults::fb2TitleFontSizeAuthor);
                mCursor->insertText(mTitleInfo->mAuthor, charFormat);

                mCursor->setPosition(topFrame->lastPosition());
                mCursor->insertBlock();
            }

            if (mTitleInfo && !mTitleInfo->mAnnotation.isEmpty()) {
                frameFormat.setBorder(0);
                mCursor->insertFrame(frameFormat);

                QTextCharFormat charFormat;
                charFormat.setFontPointSize(ZDefaults::fb2TitleFontSizeAnnotation);
                charFormat.setFontItalic(true);
                mCursor->insertText(mTitleInfo->mAnnotation, charFormat);

                mCursor->setPosition(topFrame->lastPosition());
                mCursor->insertBlock();
            }

            mCursor->insertBlock();

            if (!convertBody(element)) {
                return false;
            }
        }

        element = element.nextSiblingElement();
    }

    return true;
}

bool ZFB2Document::convertBody(const QDomElement &element)
{
    QDomElement child = element.firstChildElement();
    while (!child.isNull()) {
        if (child.tagName() == QSL("section")) {
            mCursor->insertBlock();
            if (!convertSection(child))
                return false;
        } else if (child.tagName() == QSL("image")) {
            if (!convertImage(child))
                return false;
        } else if (child.tagName() == QSL("title")) {
            if (!convertTitle(child))
                return false;
        } else if (child.tagName() == QSL("epigraph")) {
            if (!convertEpigraph(child))
                return false;
        }

        child = child.nextSiblingElement();
    }

    return true;
}

bool ZFB2Document::convertDescription(const QDomElement &element)
{
    QDomElement child = element.firstChildElement();
    while (!child.isNull()) {
        if (child.tagName() == QSL("title-info")) {
            if (!convertTitleInfo(child))
                return false;
        }

        child = child.nextSiblingElement();
    }

    return true;
}

bool ZFB2Document::convertTitleInfo(const QDomElement &element)
{
    delete mTitleInfo;
    mTitleInfo = new ZFB2TitleInfo;

    QDomElement child = element.firstChildElement();
    while (!child.isNull()) {
        if (child.tagName() == QSL("genre")) {
            QString genre;
            if (!convertTextNode(child, genre))
                return false;

            if (!genre.isEmpty())
                mTitleInfo->mGenres.append(genre);
        } else if (child.tagName() == QSL("author")) {
            QString firstName;
            QString middleName;
            QString lastName;
            QString dummy;

            if (!convertAuthor(child, firstName, middleName, lastName, dummy, dummy))
                return false;

            mTitleInfo->mAuthor = QSL("%1 %2 %3").arg(firstName, middleName, lastName);
        } else if (child.tagName() == QSL("book-title")) {
            if (!convertTextNode(child, mTitleInfo->mTitle))
                return false;
        } else if (child.tagName() == QSL("keywords")) {
            QString keywords;
            if (!convertTextNode(child, keywords))
                return false;

            mTitleInfo->mKeywords = keywords.split(QChar(u' '), Qt::SkipEmptyParts);
        } else if (child.tagName() == QSL("annotation")) {
            if (!convertAnnotation(child, mTitleInfo->mAnnotation))
                return false;
        } else if (child.tagName() == QSL("date")) {
            if (!convertDate(child, mTitleInfo->mDate))
                return false;
        } else if (child.tagName() == QSL("coverpage")) {
            mTitleInfo->mCoverPage = child;
        } else if (child.tagName() == QSL("lang")) {
            if (!convertTextNode(child, mTitleInfo->mLanguage))
                return false;
        }
        child = child.nextSiblingElement();
    }

    return true;
}

bool ZFB2Document::convertAuthor(const QDomElement &element, QString &firstName, QString &middleName,
                                 QString &lastName, QString &email, QString &nickname)
{
    QDomElement child = element.firstChildElement();
    while (!child.isNull()) {
        if (child.tagName() == QSL("first-name")) {
            if (!convertTextNode(child, firstName))
                return false;
        } else if (child.tagName() == QSL("middle-name")) {
            if (!convertTextNode(child, middleName))
                return false;
        } else if (child.tagName() == QSL("last-name")) {
            if (!convertTextNode(child, lastName))
                return false;
        } else if (child.tagName() == QSL("email")) {
            if (!convertTextNode(child, email))
                return false;
        } else if (child.tagName() == QSL("nickname")) {
            if (!convertTextNode(child, nickname))
                return false;
        }

        child = child.nextSiblingElement();
    }

    return true;
}

bool ZFB2Document::convertTextNode(const QDomElement &element, QString &data)
{
    QDomNode child = element.firstChild();
    while (!child.isNull()) {
        QDomText text = child.toText();
        if (!text.isNull())
            data = text.data();

        child = child.nextSibling();
    }

    return true;
}

bool ZFB2Document::convertDate(const QDomElement &element, QDate &date)
{
    if (element.hasAttribute(QSL("value")))
        date = QDate::fromString(element.attribute(QSL("value")), Qt::ISODate);

    return true;
}

bool ZFB2Document::convertAnnotation(const QDomElement &element, QString &data)
{
    QDomElement child = element.firstChildElement();
    while (!child.isNull()) {
        QString text = child.text();
        if (!text.isNull())
            data = child.text();

        child = child.nextSiblingElement();
    }

    return true;
}

bool ZFB2Document::convertSection(const QDomElement &element)
{
    mSectionCounter++;

    QDomElement child = element.firstChildElement();
    while (!child.isNull()) {
        if (child.tagName() == QSL("title")) {
            if (!convertTitle(child))
                return false;
        } else if (child.tagName() == QSL("epigraph")) {
            if (!convertEpigraph(child))
                return false;
        } else if (child.tagName() == QSL("image")) {
            if (!convertImage(child))
                return false;
        } else if (child.tagName() == QSL("section")) {
            if (!convertSection(child))
                return false;
        } else if (child.tagName() == QSL("p")) {
            QTextBlockFormat format;
            format.setTextIndent(ZDefaults::fb2ParagraphIndent);
            mCursor->insertBlock(format);
            if (!convertParagraph(child))
                return false;
        } else if (child.tagName() == QSL("poem")) {
            if (!convertPoem(child))
                return false;
        } else if (child.tagName() == QSL("subtitle")) {
            if (!convertSubTitle(child))
                return false;
        } else if (child.tagName() == QSL("cite")) {
            if (!convertCite(child))
                return false;
        } else if (child.tagName() == QSL("empty-line")) {
            if (!convertEmptyLine(child))
                return false;
        } else if (child.tagName() == QSL("code")) {
            if (!convertCode(child))
                return false;
        } else if (child.tagName() == QSL("table")) {
            if (!convertTable(child))
                return false;
        }

        child = child.nextSiblingElement();
    }

    mSectionCounter--;

    return true;
}

bool ZFB2Document::convertTitle(const QDomElement &element)
{
    QTextFrame *topFrame = mCursor->currentFrame();

    QTextFrameFormat frameFormat;
    frameFormat.setBorder(ZDefaults::fb2TitleBorder);
    frameFormat.setPadding(ZDefaults::fb2TitlePadding);
    frameFormat.setBackground(ZDefaults::fb2TitleBackground);

    mCursor->insertFrame(frameFormat);

    QDomElement child = element.firstChildElement();

    bool firstParagraph = true;
    while (!child.isNull()) {
        if (child.tagName() == QSL("p")) {
            if (firstParagraph) {
                firstParagraph = false;
            } else {
                mCursor->insertBlock();
            }

            QTextCharFormat origFormat = mCursor->charFormat();

            QTextCharFormat titleFormat(origFormat);
            titleFormat.setFontPointSize(ZDefaults::fb2TitleFontSizeTitle - (mSectionCounter * 2));
            titleFormat.setFontWeight(QFont::Bold);
            mCursor->setCharFormat(titleFormat);

            if (!convertParagraph(child))
                return false;

            mCursor->setCharFormat(origFormat);

        } else if (child.tagName() == QSL("empty-line")) {
            if (!convertEmptyLine(child))
                return false;
        }

        child = child.nextSiblingElement();
    }

    mCursor->setPosition(topFrame->lastPosition());

    return true;
}

bool ZFB2Document::convertParagraph(const QDomElement &element)
{
    QDomNode child = element.firstChild();
    while (!child.isNull()) {
        if (child.isElement()) {
            const QDomElement childElement = child.toElement();
            if (childElement.tagName() == QSL("emphasis")) {
                if (!convertEmphasis(childElement))
                    return false;
            } else if (childElement.tagName() == QSL("strong")) {
                if (!convertStrong(childElement))
                    return false;
            } else if (childElement.tagName() == QSL("style")) {
                if (!convertStyle(childElement))
                    return false;
            } else if (childElement.tagName() == QSL("a")) {
                if (!convertLink(childElement))
                    return false;
            } else if (childElement.tagName() == QSL("image")) {
                if (!convertImage(childElement))
                    return false;
            } else if (childElement.tagName() == QSL("strikethrough")) {
                if (!convertStrikethrough(childElement))
                    return false;
            } else if (childElement.tagName() == QSL("code")) {
                if (!convertCode(childElement))
                    return false;
            } else if (childElement.tagName() == QSL("sup")) {
                if (!convertSuperScript(childElement))
                    return false;
            } else if (childElement.tagName() == QSL("sub")) {
                if (!convertSubScript(childElement))
                    return false;
            }
        } else if (child.isText()) {
            const QDomText childText = child.toText();
            mCursor->insertText(childText.data());
        }

        child = child.nextSibling();
    }

    return true;
}

bool ZFB2Document::convertEmphasis(const QDomElement &element)
{
    QTextCharFormat origFormat = mCursor->charFormat();

    QTextCharFormat italicFormat(origFormat);
    italicFormat.setFontItalic(true);
    mCursor->setCharFormat(italicFormat);

    if (!convertParagraph(element))
        return false;

    mCursor->setCharFormat(origFormat);

    return true;
}

bool ZFB2Document::convertStrikethrough(const QDomElement &element)
{
    QTextCharFormat origFormat = mCursor->charFormat();

    QTextCharFormat strikeoutFormat(origFormat);
    strikeoutFormat.setFontStrikeOut(true);
    mCursor->setCharFormat(strikeoutFormat);

    if (!convertParagraph(element))
        return false;

    mCursor->setCharFormat(origFormat);

    return true;
}

bool ZFB2Document::convertStrong(const QDomElement &element)
{
    QTextCharFormat origFormat = mCursor->charFormat();

    QTextCharFormat boldFormat(origFormat);
    boldFormat.setFontWeight(QFont::Bold);
    mCursor->setCharFormat(boldFormat);

    if (!convertParagraph(element))
        return false;

    mCursor->setCharFormat(origFormat);

    return true;
}

bool ZFB2Document::convertStyle(const QDomElement &element)
{
    return convertParagraph(element);
}

bool ZFB2Document::convertBinary(const QDomElement &element)
{
    const QString id = element.attribute(QSL("id"));

    const QDomText textNode = element.firstChild().toText();
    QByteArray data = textNode.data().toLatin1();
    data = QByteArray::fromBase64(data);

    addResource(QTextDocument::ImageResource, QUrl(id), QImage::fromData(data));

    return true;
}

bool ZFB2Document::convertCover(const QDomElement &element)
{
    QDomElement child = element.firstChildElement();
    while (!child.isNull()) {
        if (child.tagName() == QSL("image")) {
            if (!convertImage(child))
                return false;
        }

        child = child.nextSiblingElement();
    }

    return true;
}

bool ZFB2Document::convertImage(const QDomElement &element)
{
    QString href = element.attributeNS(QSL("http://www.w3.org/1999/xlink"), QSL("href"));

    if (href.startsWith(QChar(u'#')))
        href = href.mid(1);

    const auto img = qvariant_cast<QImage>(resource(QTextDocument::ImageResource, QUrl(href)));

    QTextImageFormat format;
    format.setName(href);

    if (img.width() > maxContentWidth())
        format.setWidth(maxContentWidth());

    format.setHeight(img.height());

    mCursor->insertImage(format);

    return true;
}

bool ZFB2Document::convertEpigraph(const QDomElement &element)
{
    QDomElement child = element.firstChildElement();
    while (!child.isNull()) {
        if (child.tagName() == QSL("p")) {
            QTextBlockFormat format;
            format.setTextIndent(ZDefaults::fb2ParagraphIndent);
            mCursor->insertBlock(format);
            if (!convertParagraph(child))
                return false;
        } else if (child.tagName() == QSL("poem")) {
            if (!convertPoem(child))
                return false;
        } else if (child.tagName() == QSL("cite")) {
            if (!convertCite(child))
                return false;
        } else if (child.tagName() == QSL("empty-line")) {
            if (!convertEmptyLine(child))
                return false;
        } else if (child.tagName() == QSL("text-author")) {
            QTextBlockFormat format;
            format.setTextIndent(ZDefaults::fb2AuthorParagraphIndent);
            mCursor->insertBlock(format);
            if (!convertParagraph(child))
                return false;
        }

        child = child.nextSiblingElement();
    }

    return true;
}

bool ZFB2Document::convertPoem(const QDomElement &element)
{
    QDomElement child = element.firstChildElement();
    while (!child.isNull()) {
        if (child.tagName() == QSL("title")) {
            if (!convertTitle(child))
                return false;
        } else if (child.tagName() == QSL("epigraph")) {
            if (!convertEpigraph(child))
                return false;
        } else if (child.tagName() == QSL("empty-line")) {
            if (!convertEmptyLine(child))
                return false;
        } else if (child.tagName() == QSL("stanza")) {
            if (!convertStanza(child))
                return false;
        } else if (child.tagName() == QSL("text-author")) {
            QTextBlockFormat format;
            format.setTextIndent(ZDefaults::fb2AuthorParagraphIndent);
            mCursor->insertBlock(format);
            if (!convertParagraph(child))
                return false;
        }

        child = child.nextSiblingElement();
    }

    return true;
}

bool ZFB2Document::convertSubTitle(const QDomElement &element)
{
    QTextFrame *topFrame = mCursor->currentFrame();

    QTextFrameFormat frameFormat;
    frameFormat.setBorder(ZDefaults::fb2TitleBorder);
    frameFormat.setPadding(ZDefaults::fb2TitlePadding);
    frameFormat.setBackground(ZDefaults::fb2TitleBackground);
    frameFormat.setTopMargin(ZDefaults::fb2SubTitleMargin);

    mCursor->insertFrame(frameFormat);

    if (!convertParagraph(element)) {
        return false;
    }

    mCursor->setPosition(topFrame->lastPosition());

    return true;
}

bool ZFB2Document::convertCite(const QDomElement &element)
{
    QDomElement child = element.firstChildElement();
    while (!child.isNull()) {
        if (child.tagName() == QSL("p")) {
            QTextBlockFormat format;
            format.setTextIndent(ZDefaults::fb2ParagraphIndent);
            mCursor->insertBlock(format);
            if (!convertParagraph(child))
                return false;
        } else if (child.tagName() == QSL("poem")) {
            if (!convertParagraph(child))
                return false;
        } else if (child.tagName() == QSL("text-author")) {
            QTextBlockFormat format;
            format.setTextIndent(ZDefaults::fb2AuthorParagraphIndent);
            mCursor->insertBlock(format);
            if (!convertParagraph(child))
                return false;
        } else if (child.tagName() == QSL("empty-line")) {
            if (!convertEmptyLine(child))
                return false;
        } else if (child.tagName() == QSL("subtitle")) {
            if (!convertSubTitle(child))
                return false;
        } else if (child.tagName() == QSL("table")) {
            if (!convertTable(child))
                return false;
        }

        child = child.nextSiblingElement();
    }

    return true;
}

bool ZFB2Document::convertEmptyLine(const QDomElement &element)
{
    Q_UNUSED(element)
    mCursor->insertText(QSL("\n\n"));
    return true;
}

bool ZFB2Document::convertLink(const QDomElement &element)
{
//    QString href = element.attributeNS(QSL("http://www.w3.org/1999/xlink"), QSL("href"));
    QString type = element.attributeNS(QSL("http://www.w3.org/1999/xlink"), QSL("type"));

    if (type == QSL("note"))
        mCursor->insertText(QSL("["));

    QTextCharFormat origFormat(mCursor->charFormat());

    QTextCharFormat format(mCursor->charFormat());
    format.setForeground(Qt::blue);
    format.setFontUnderline(true);
    mCursor->setCharFormat(format);

    QDomNode child = element.firstChild();
    while (!child.isNull()) {
        if (child.isElement()) {
            const QDomElement childElement = child.toElement();
            if (childElement.tagName() == QSL("emphasis")) {
                if (!convertEmphasis(childElement))
                    return false;
            } else if (childElement.tagName() == QSL("strong")) {
                if (!convertStrong(childElement))
                    return false;
            } else if (childElement.tagName() == QSL("style")) {
                if (!convertStyle(childElement))
                    return false;
            }
        } else if (child.isText()) {
            const QDomText text = child.toText();
            if (!text.isNull()) {
                mCursor->insertText(text.data());
            }
        }

        child = child.nextSibling();
    }
    mCursor->setCharFormat(origFormat);

    if (type == QSL("note"))
        mCursor->insertText(QSL("]"));

    return true;
}

bool ZFB2Document::convertStanza(const QDomElement &element)
{
    QDomElement child = element.firstChildElement();
    while (!child.isNull()) {
        if (child.tagName() == QSL("title")) {
            if (!convertTitle(child))
                return false;
        } else if (child.tagName() == QSL("subtitle")) {
            if (!convertSubTitle(child))
                return false;
        } else if (child.tagName() == QSL("v")) {
            QTextBlockFormat format;
            format.setTextIndent(ZDefaults::fb2StanzaIndent);
            mCursor->insertBlock(format);
            if (!convertParagraph(child))
                return false;
        }

        child = child.nextSiblingElement();
    }

    return true;
}

bool ZFB2Document::convertCode(const QDomElement &element)
{
    QTextCharFormat origFormat = mCursor->charFormat();

    QTextCharFormat codeFormat(origFormat);
    codeFormat.setFontFamilies({QSL("monospace")});
    mCursor->setCharFormat(codeFormat);

    if (!convertParagraph(element))
        return false;

    mCursor->setCharFormat(origFormat);

    return true;
}

bool ZFB2Document::convertSuperScript(const QDomElement &element)
{
    QTextCharFormat origFormat = mCursor->charFormat();

    QTextCharFormat superScriptFormat(origFormat);
    superScriptFormat.setVerticalAlignment(QTextCharFormat::AlignSuperScript);
    mCursor->setCharFormat(superScriptFormat);

    if (!convertParagraph(element))
        return false;

    mCursor->setCharFormat(origFormat);

    return true;
}

bool ZFB2Document::convertSubScript(const QDomElement &element)
{
    QTextCharFormat origFormat = mCursor->charFormat();

    QTextCharFormat subScriptFormat(origFormat);
    subScriptFormat.setVerticalAlignment(QTextCharFormat::AlignSubScript);
    mCursor->setCharFormat(subScriptFormat);

    if (!convertParagraph(element))
        return false;

    mCursor->setCharFormat(origFormat);

    return true;
}

bool ZFB2Document::convertTable(const QDomElement &element)
{
    QTextFrame *topFrame = mCursor->currentFrame();

    QTextTable *table = nullptr;

    QDomElement child = element.firstChildElement();
    while (!child.isNull()) {
        if (child.tagName() == QSL("tr")) {
            if (table) {
                table->appendRows(1);
            } else {
                QTextTableFormat tableFormat;
                tableFormat.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
                tableFormat.setBorderCollapse(true);
                table = mCursor->insertTable(1, 1, tableFormat);
            }

            if (!convertTableRow(child, *table))
                return false;
        }

        child = child.nextSiblingElement();
    }

    mCursor->setPosition(topFrame->lastPosition());

    return true;
}

bool ZFB2Document::convertTableRow(const QDomElement &element, QTextTable &table)
{
    QDomElement child = element.firstChildElement();
    int column = 0;
    while (!child.isNull()) {
        if (child.tagName() == QSL("th")) {
            if (!convertTableHeaderCell(child, table, column))
                return false;
        } else if (child.tagName() == QSL("td")) {
            if (!convertTableCell(child, table, column))
                return false;
        }

        child = child.nextSiblingElement();
    }

    return true;
}

bool ZFB2Document::convertTableHeaderCell(const QDomElement &element, QTextTable &table, int &column)
{
    QTextCharFormat charFormat;
    charFormat.setFontWeight(QFont::Bold);
    return convertTableCellHelper(element, table, column, charFormat);
}

bool ZFB2Document::convertTableCell(const QDomElement &element, QTextTable &table, int &column)
{
    QTextCharFormat charFormat;
    return convertTableCellHelper(element, table, column, charFormat);
}

bool ZFB2Document::convertTableCellHelper(const QDomElement &element, QTextTable &table, int &column,
                                          const QTextCharFormat &charFormat)
{
    int row = table.rows() - 1;

    int colspan = qMax(element.attribute(QSL("colspan")).toInt(), 1);

    int columnsToAppend = column + colspan - table.columns();
    if (columnsToAppend > 0) {
        table.appendColumns(columnsToAppend);
    }

    table.mergeCells(row, column, 1, colspan);

    int cellCursorPosition = table.cellAt(row, column).firstPosition();
    mCursor->setPosition(cellCursorPosition);

    Qt::Alignment alignment;

    QString halign = element.attribute(QSL("halign"));
    if (halign == QSL("center")) {
        alignment |= Qt::AlignHCenter;
    } else if (halign == QSL("right")) {
        alignment |= Qt::AlignRight;
    } else {
        alignment |= Qt::AlignLeft;
    }

    QString valign = element.attribute(QSL("valign"));
    if (valign == QSL("center")) {
        alignment |= Qt::AlignVCenter;
    } else if (valign == QSL("bottom")) {
        alignment |= Qt::AlignBottom;
    } else {
        alignment |= Qt::AlignTop;
    }

    QTextBlockFormat format;
    format.setAlignment(alignment);
    mCursor->insertBlock(format, charFormat);

    if (!convertParagraph(element))
        return false;

    column += colspan;
    return true;
}
