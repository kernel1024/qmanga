#include "zpdfreader.h"

#ifdef WITH_POPPLER
#include <poppler-page.h>

using namespace poppler;
#endif

ZPdfReader::ZPdfReader(QObject *parent, QString filename) :
    ZAbstractReader(parent,filename)
{
#ifdef WITH_POPPLER
    doc = NULL;
    renderer = new page_renderer();
#endif
}

ZPdfReader::~ZPdfReader()
{
    closeFile();
    delete renderer;
}

bool ZPdfReader::openFile()
{
#ifdef WITH_POPPLER
    if (opened)
        return false;

    sortList.clear();

    QFileInfo fi(fileName);
    if (!fi.isFile() || !fi.isReadable()) {
        return false;
    }

    doc = document::load_from_file(fileName.toStdString());

    if (doc==NULL)
        return false;

    for (int i=0;i<doc->pages();i++) {
        sortList << ZFileEntry(QString("%1").arg(i,6,10,QChar('0')),i);
    }

    qSort(sortList);

    opened = true;
    return true;
#else
    return false;
#endif
}

void ZPdfReader::closeFile()
{
#ifdef WITH_POPPLER
    if (!opened)
        return;

    if (doc!=NULL)
        delete doc;
    doc = NULL;
#endif

    opened = false;
    sortList.clear();
}

QByteArray ZPdfReader::loadPage(int num)
{
    QByteArray res;
    res.clear();
#ifdef WITH_POPPLER
    if (!opened || doc==NULL)
        return res;

    if (num<0 || num>=sortList.count()) return res;

    int idx = sortList.at(num).idx;
    page* p = doc->create_page(idx);
    if (p!=NULL) {
        double dpi = 72.0*((double)zg->preferredWidth)/p->page_rect().width();
        image img = renderer->render_page(p,dpi,dpi);
        QImage qimg;
        if (img.is_valid()) {
            if (img.format()==image::format_mono)
                qimg = QImage((uchar*)img.data(),img.width(),img.height(),QImage::Format_Mono);
            else if (img.format()==image::format_rgb24)
                qimg = QImage((uchar*)img.data(),img.width(),img.height(),QImage::Format_RGB888);
            else if (img.format()==image::format_argb32)
                qimg = QImage((uchar*)img.data(),img.width(),img.height(),QImage::Format_ARGB32);
        }
        delete p;
        QBuffer buf(&res);
        buf.open(QIODevice::WriteOnly);
        qimg.save(&buf,"BMP");
        qimg = QImage();
    }
#endif
    return res;
}

QString ZPdfReader::getMagic()
{
    return QString("PDF");
}
