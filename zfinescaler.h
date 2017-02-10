#ifndef ZFINESCALER_H
#define ZFINESCALER_H

#include <QObject>
#include <QImage>
#include <QSize>
#include "scalefilter.h"
#include "zmangaview.h"

class ZFineScaler : public QObject
{
    Q_OBJECT
private:
    QImage m_image;
    QSize m_targetSize;
    Blitz::ScaleFilterType m_filter;
    int m_page;
    ZMangaView *m_view;

    bool m_abort;

public:
    explicit ZFineScaler(QObject *parent = 0);
    void setSource(const QImage& image, const QSize& targetSize, int page,
                   Blitz::ScaleFilterType filter, ZMangaView *view);
    ~ZFineScaler();
signals:
    void finished();

public slots:
    void resample();
    void abortScaling(int num, QString msg);


};

#endif // ZFINESCALER_H
