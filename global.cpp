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

QString formatSize(qint64 size)
{
    if (size<1024) return QString("%1 bytes").arg(size);
    else if (size<1024*1024) return QString("%1 Kb").arg(size/1024);
    else if (size<1024*1024*1024) return QString("%1 Mb").arg(size/(1024*1024));
    else return QString("%1 Gb").arg(size/(1024*1024*1024));
}
