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

#ifndef ZEPUBDOCUMENT_H
#define ZEPUBDOCUMENT_H

#include <QObject>
#include <QTextDocument>
#include "zabstracttextdocument.h"

#ifdef WITH_EPUB
#include <epub.h>

class ZEpubDocument : public ZAbstractTextDocument
{
    Q_OBJECT
    Q_DISABLE_COPY(ZEpubDocument)

public:
    ZEpubDocument(QObject* parent, const QString &fileName);
    ~ZEpubDocument() override;
    void setCurrentSubDocument(const QString &doc);

    bool isValid() override;

protected:
    QVariant loadResource(int type, const QUrl &name) override;

private:
    struct epub *m_epub;
    QUrl m_currentSubDocument;

    bool epubConvert();
    void epubClean();
    QString checkCSS(const QString &css);
};

#endif // WITH_EPUB

#endif // ZEPUBDOCUMENT_H
