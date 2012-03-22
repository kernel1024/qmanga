#include "global.h"
#include "zabstractreader.h"
#include "zzipreader.h"
#include "zglobal.h"

ZAbstractReader *readerFactory(QObject* parent, QString filename)
{
    QString mime = zGlobal->detectMIME(filename).toLower();
    if (mime.contains("application/zip",Qt::CaseInsensitive)) {
        return new ZZipReader(parent,filename);
    } else {
        return NULL;
    }
}

