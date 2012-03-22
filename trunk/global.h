#ifndef GLOBAL_H
#define GLOBAL_H

#include <QtCore>

#ifndef ARGUNUSED
#define ARGUNUSED(x) (void)(x)
#endif

#define maxPreviewSize 500

typedef QHash<int,QImage> QImageHash;
typedef QList<int> QIntList;

class ZAbstractReader;

extern ZAbstractReader *readerFactory(QObject* parent, QString filename);

#endif // GLOBAL_H
