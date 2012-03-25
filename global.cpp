#include "global.h"
#include "zabstractreader.h"
#include "zzipreader.h"
#include "zrarreader.h"
#include "zpdfreader.h"
#include "zglobal.h"

ZAbstractReader *readerFactory(QObject* parent, QString filename)
{
    QString mime = zGlobal->detectMIME(filename).toLower();
    if (mime.contains("application/zip",Qt::CaseInsensitive)) {
        return new ZZipReader(parent,filename);
    } else if (mime.contains("application/x-rar",Qt::CaseInsensitive) ||
               mime.contains("application/rar",Qt::CaseInsensitive)) {
        return new ZRarReader(parent,filename);
    } else if (mime.contains("application/pdf",Qt::CaseInsensitive)) {
        return new ZPdfReader(parent,filename);
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
    qDebug() << ref1 << ref2;
    // find common part of both strings
    int mlen = qMin(ref1.length(),ref2.length());
    int diffs = 0;
    for (int i=0;i<mlen;i++) {
        if (ref1.at(i).isDigit() || ref2.at(i).isDigit()) {
            diffs = i;
            break;
        }
        if (ref1.at(i)!=ref2.at(i)) {
            diffs = i;
            break;
        }
    }
    if (diffs>0) // and compare only differences as strings again
        return compareWithNumerics(ref1.mid(diffs),ref2.mid(diffs));

    // try convert to int
    bool okconv1, okconv2;
    int v1 = ref1.toInt(&okconv1);
    int v2 = ref2.toInt(&okconv2);
    if (okconv1 && okconv2) {
        if (v1<v2) return -1;
        else if (v1==v2) return 0;
        else return 1;
    }

    // simply compare strings
    return QString::compare(ref1,ref2);
}
