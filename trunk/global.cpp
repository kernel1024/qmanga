#include "global.h"
#include "zabstractreader.h"
#include "zzipreader.h"
#include "zrarreader.h"
#include "zpdfreader.h"
#include "zglobal.h"
#include <ImageMagick/Magick++.h>

using namespace Magick;

ZAbstractReader *readerFactory(QObject* parent, QString filename)
{
    QString mime = detectMIME(filename).toLower();
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

#ifdef QB_KDEDIALOGS
QString getKDEFilters(const QString & qfilter)
{
    QStringList f = qfilter.split(";;",QString::SkipEmptyParts);
    QStringList r;
    r.clear();
    for (int i=0;i<f.count();i++) {
        QString s = f.at(i);
        if (s.indexOf('(')<0) continue;
        QString ftitle = s.left(s.indexOf('(')).trimmed();
        s.remove(0,s.indexOf('(')+1);
        if (s.indexOf(')')<0) continue;
        s = s.left(s.indexOf(')'));
        if (s.isEmpty()) continue;
        QStringList e = s.split(' ');
        for (int j=0;j<e.count();j++) {
            if (r.contains(e.at(j),Qt::CaseInsensitive)) continue;
            if (ftitle.isEmpty())
                r << QString("%1").arg(e.at(j));
            else
                r << QString("%1|%2").arg(e.at(j)).arg(ftitle);
        }
    }

    int idx;
    r.sort();
    idx=r.indexOf(QRegExp("^\\*\\.jpg.*",Qt::CaseInsensitive));
    if (idx>=0) { r.swap(0,idx); }
    idx=r.indexOf(QRegExp("^\\*\\.png.*",Qt::CaseInsensitive));
    if (idx>=0) { r.swap(1,0); r.swap(0,idx); }

    QString sf = r.join("\n");
    return sf;
}
#endif

QString getOpenFileNameD ( QWidget * parent, const QString & caption, const QString & dir, const QString & filter, QString * selectedFilter, QFileDialog::Options options )
{
#ifdef QB_KDEDIALOGS
    ARGUNUSED(selectedFilter);
    ARGUNUSED(options);
    return KFileDialog::getOpenFileName(KUrl(dir),getKDEFilters(filter),parent,caption);
#else
    return QFileDialog::getOpenFileName(parent,caption,dir,filter,selectedFilter,options);
#endif
}

QStringList getOpenFileNamesD ( QWidget * parent, const QString & caption, const QString & dir, const QString & filter, QString * selectedFilter, QFileDialog::Options options )
{
#ifdef QB_KDEDIALOGS
    ARGUNUSED(selectedFilter);
    ARGUNUSED(options);
    return KFileDialog::getOpenFileNames(KUrl(dir),getKDEFilters(filter),parent,caption);
#else
    return QFileDialog::getOpenFileNames(parent,caption,dir,filter,selectedFilter,options);
#endif
}

QString getSaveFileNameD ( QWidget * parent, const QString & caption, const QString & dir, const QString & filter, QString * selectedFilter, QFileDialog::Options options )
{
#ifdef QB_KDEDIALOGS
    ARGUNUSED(selectedFilter);
    ARGUNUSED(options);
    return KFileDialog::getSaveFileName(KUrl(dir),getKDEFilters(filter),parent,caption);
#else
    return QFileDialog::getSaveFileName(parent,caption,dir,filter,selectedFilter,options);
#endif
}

QString	getExistingDirectoryD ( QWidget * parent, const QString & caption, const QString & dir, QFileDialog::Options options )
{
#ifdef QB_KDEDIALOGS
    ARGUNUSED(options);
    return KFileDialog::getExistingDirectory(KUrl(dir),parent,caption);
#else
    return QFileDialog::getExistingDirectory(parent,caption,dir,options);
#endif
}

QString detectMIME(QString filename)
{
    magic_t myt = magic_open(MAGIC_ERROR|MAGIC_MIME_TYPE);
    magic_load(myt,NULL);
    QByteArray bma = filename.toUtf8();
    const char* bm = bma.data();
    const char* mg = magic_file(myt,bm);
    if (mg==NULL) {
        qDebug() << "libmagic error: " << magic_errno(myt) << QString::fromUtf8(magic_error(myt));
        return QString("text/plain");
    }
    QString mag(mg);
    magic_close(myt);
    return mag;
}

QString detectMIME(QByteArray buf)
{
    magic_t myt = magic_open(MAGIC_ERROR|MAGIC_MIME_TYPE);
    magic_load(myt,NULL);
    const char* mg = magic_buffer(myt,buf.data(),buf.length());
    if (mg==NULL) {
        qDebug() << "libmagic error: " << magic_errno(myt) << QString::fromUtf8(magic_error(myt));
        return QString("text/plain");
    }
    QString mag(mg);
    magic_close(myt);
    return mag;
}

QPixmap resizeImage(QPixmap src, QSize targetSize, bool forceFilter, Z::ResizeFilter filter)
{
    Z::ResizeFilter rf = zg->resizeFilter;
    if (forceFilter)
        rf = filter;
    if (rf==Z::Nearest)
        return src.scaled(targetSize,Qt::IgnoreAspectRatio,Qt::FastTransformation);
    else if (rf==Z::Bilinear)
        return src.scaled(targetSize,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    else {
        QTime tmr;
        tmr.start();

        // Load QImage to ImageMagick image
        QByteArray bufSrc;
        QBuffer buf(&bufSrc);
        buf.open(QIODevice::WriteOnly);
        src.save(&buf,"BMP");
        buf.close();
        Blob iBlob(bufSrc.data(),bufSrc.size());
        Image iImage(iBlob,Geometry(src.width(),src.height()),"BMP");

        // Resize image
        if (rf==Z::Lanczos)
            iImage.filterType(LanczosFilter);
        else if (rf==Z::Gaussian)
            iImage.filterType(GaussianFilter);
        else if (rf==Z::Lanczos2)
            iImage.filterType(Lanczos2Filter);
        else if (rf==Z::Cubic)
            iImage.filterType(CubicFilter);
        else if (rf==Z::Sinc)
            iImage.filterType(SincFilter);
        else if (rf==Z::Triangle)
            iImage.filterType(TriangleFilter);
        else if (rf==Z::Mitchell)
            iImage.filterType(MitchellFilter);
        else
            iImage.filterType(LanczosFilter);

        iImage.resize(Geometry(targetSize.width(),targetSize.height()));

        // Convert image to QImage
        Blob oBlob;
        iImage.magick("BMP");
        iImage.write(&oBlob);
        bufSrc.clear();
        bufSrc=QByteArray::fromRawData((char*)(oBlob.data()),oBlob.length());
        QPixmap dst;
        dst.loadFromData(bufSrc);
        bufSrc.clear();

        //qDebug() << "ImageMagick ms: " << tmr.elapsed();
        return dst;
    }
}
