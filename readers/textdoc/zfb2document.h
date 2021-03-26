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

#ifndef ZFB2DOCUMENT_H
#define ZFB2DOCUMENT_H

#include <QObject>
#include <QDomElement>
#include <QTextTable>
#include <QTextCursor>
#include "zabstracttextdocument.h"

class ZFB2TitleInfo;

class ZFB2Document : public ZAbstractTextDocument
{
    Q_OBJECT
    Q_DISABLE_COPY(ZFB2Document)
public:
    ZFB2Document(QObject* parent, const QString &fileName);
    ~ZFB2Document() override;

    bool isValid() override;

private:
    bool m_loadedOk { false };
    int mSectionCounter { 0 };
    QTextCursor *mCursor { nullptr };
    ZFB2TitleInfo *mTitleInfo { nullptr };

    bool fb2Convert(const QByteArray& data);

    bool convertBody(const QDomElement &element);
    bool convertDescription(const QDomElement &element);
    bool convertSection(const QDomElement &element);
    bool convertTitle(const QDomElement &element);
    bool convertParagraph(const QDomElement &element);
    bool convertBinary(const QDomElement &element);
    bool convertCover(const QDomElement &element);
    bool convertImage(const QDomElement &element);
    bool convertEpigraph(const QDomElement &element);
    bool convertPoem(const QDomElement &element);
    bool convertSubTitle(const QDomElement &element);
    bool convertCite(const QDomElement &element);
    bool convertEmptyLine(const QDomElement &element);
    bool convertLink(const QDomElement &element);
    bool convertEmphasis(const QDomElement &element);
    bool convertStrong(const QDomElement &element);
    bool convertStrikethrough(const QDomElement &element);
    bool convertStyle(const QDomElement &element);
    bool convertStanza(const QDomElement &element);
    bool convertCode(const QDomElement &element);
    bool convertSuperScript(const QDomElement &element);
    bool convertSubScript(const QDomElement &element);
    bool convertTable(const QDomElement &element);
    bool convertTableRow(const QDomElement &element, QTextTable &table);
    bool convertTableHeaderCell(const QDomElement &element, QTextTable &table, int &column);
    bool convertTableCell(const QDomElement &element, QTextTable &table, int &column);
    bool convertTableCellHelper(const QDomElement &element, QTextTable &table, int &column,
                                const QTextCharFormat &charFormat);
    bool convertTitleInfo(const QDomElement &element);
    bool convertAuthor(const QDomElement &element, QString &firstName, QString &middleName,
                       QString &lastName, QString &email, QString &nickname);
    bool convertDate(const QDomElement &element, QDate &date);
    bool convertTextNode(const QDomElement &element, QString &data);
    bool convertAnnotation(const QDomElement &element, QString &data);
};

#endif // ZFB2DOCUMENT_H
