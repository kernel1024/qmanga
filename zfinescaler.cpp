#include <QThread>
#include <QDebug>
#include "zfinescaler.h"
#include "global.h"

ZFineScaler::ZFineScaler(QObject *parent)
    : QObject(parent)
{
    m_image = QImage();
    m_targetSize = QSize();
    m_filter = Blitz::UndefinedFilter;
    m_page = -1;
    m_view = NULL;
}

void ZFineScaler::setSource(const QImage &image, const QSize &targetSize, int page,
                            Blitz::ScaleFilterType filter, ZMangaView* view)
{
    m_image = image;
    m_targetSize = targetSize;
    m_filter = filter;
    m_page = page;
    m_view = view;
}

ZFineScaler::~ZFineScaler()
{
}

void ZFineScaler::resample()
{
    m_abort = false;

    QImage res = resizeImage(m_image,m_targetSize,true,m_filter,&m_abort);

    if (m_view!=NULL && !res.isNull())
        QMetaObject::invokeMethod(m_view,"redrawPageEx",Qt::QueuedConnection,
                                  Q_ARG(QImage, res),
                                  Q_ARG(int, m_page));
    emit finished();
}

void ZFineScaler::abortScaling(int num, QString msg)
{
    Q_UNUSED(msg)

    if (num!=m_page)
        m_abort = true;
}
