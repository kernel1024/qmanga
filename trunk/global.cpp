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

QString escapeParam(QString param)
{
    QString res = param;
    res.replace("'","\\'");
    res.replace("\"","\\\"");
    res.replace(";","");
    res.replace("%","");
    return res;
}

int compareWithNumerics(QString ref1, QString ref2)
{
    bool okconv1, okconv2;
    int v1 = ref1.toInt(&okconv1);
    int v2 = ref2.toInt(&okconv2);
    if (okconv1 && okconv2) {
        if (v1<v2) return -1;
        else if (v1==v2) return 0;
        else return 1;
    }
    return QString::compare(ref1,ref2);
}
