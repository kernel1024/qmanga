#include <QBuffer>
#include "zsingleimagereader.h"

ZSingleImageReader::ZSingleImageReader(QObject *parent, const QString &filename) :
    ZAbstractReader(parent,filename)
{
}

bool ZSingleImageReader::openFile()
{
    if (isOpened())
        return false;

    QString fileName = getFileName();
    if (!fileName.startsWith(QSL(":CLIP:"))) {
        if (!m_page.load(fileName)) {
            m_page = QImage();
            return false;
        }
    } else {
        fileName.remove(0,QSL(":CLIP:").length());
        QByteArray imgs = QByteArray::fromBase64(fileName.toLatin1());
        fileName = QSL("clipboard");
        QBuffer buf(&imgs);
        bool loaded = m_page.load(&buf,"BMP");
        buf.close();
        imgs.clear();
        if (m_page.isNull() || !loaded)
            return false;
    }

    addSortEntry(fileName,0);
    performListSort();
    setOpenFileSuccess();
    return true;
}

void ZSingleImageReader::closeFile()
{
    if (!isOpened())
        return;

    m_page = QImage();
    ZAbstractReader::closeFile();
}

QByteArray ZSingleImageReader::loadPage(int num)
{
    Q_UNUSED(num)

    QByteArray res;
    if (!isOpened())
        return res;

    QBuffer buf(&res);
    buf.open(QIODevice::WriteOnly);
    m_page.save(&buf,"BMP");

    return res;
}

QImage ZSingleImageReader::loadPageImage(int num)
{
    Q_UNUSED(num)

    if (!isOpened())
        return QImage();

    return m_page;
}

QString ZSingleImageReader::getMagic()
{
    return QSL("AUX");
}
