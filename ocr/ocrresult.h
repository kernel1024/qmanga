#ifndef ZOCRRESULT_H
#define ZOCRRESULT_H

#include <QString>
#include <QPolygonF>
#include <QSize>
#include <QList>
#include <QMetaType>

namespace Z {

struct ZOCRTextRegion
{
    QString text;
    QString translation;

    QPolygonF polygon;
    float confidence = -1.0F;

    QRectF boundingRect() const
    {
        return polygon.boundingRect();
    }
};

struct ZOCRPageResult
{
    QSize imageSize;
    QList<ZOCRTextRegion> regions;

    bool isEmpty() const
    {
        return regions.isEmpty();
    }
};
}

Q_DECLARE_METATYPE(Z::ZOCRTextRegion)
Q_DECLARE_METATYPE(Z::ZOCRPageResult)

#endif
