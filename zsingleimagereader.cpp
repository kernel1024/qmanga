#include <QBuffer>
#include "zsingleimagereader.h"

ZSingleImageReader::ZSingleImageReader(QObject *parent, const QString &filename) :
    ZAbstractReader(parent,filename)
{
    page = QImage();
}

bool ZSingleImageReader::openFile()
{
    if (opened)
        return false;

    if (!fileName.startsWith(":CLIP:")) {
        if (!page.load(fileName)) {
            page = QImage();
            return false;
        }
    } else {
        fileName.remove(0,QString(":CLIP:").length());
        QByteArray imgs = QByteArray::fromBase64(fileName.toLatin1());
        fileName = "clipboard";
        QBuffer buf(&imgs);
        bool loaded = page.load(&buf,"BMP");
        buf.close();
        imgs.clear();
        if (page.isNull() || !loaded)
            return false;
    }
    sortList << ZFileEntry(fileName,0);
    opened = true;
    return true;
}

void ZSingleImageReader::closeFile()
{
    if (!opened)
        return;

    page = QImage();
    opened = false;
    sortList.clear();
}

QByteArray ZSingleImageReader::loadPage(int)
{
    QByteArray res;
    res.clear();
    if (!opened)
        return res;

    QBuffer buf(&res);
    buf.open(QIODevice::WriteOnly);
    page.save(&buf,"BMP");

    return res;
}

QImage ZSingleImageReader::loadPageImage(int)
{
    if (!opened)
        return QImage();

    return page;
}

QString ZSingleImageReader::getMagic()
{
    return QString("AUX");
}

QString ZSingleImageReader::getInternalPath(int)
{
    return QString();
}
