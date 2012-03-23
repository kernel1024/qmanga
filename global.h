#ifndef GLOBAL_H
#define GLOBAL_H

#include <QtCore>

#ifndef ARGUNUSED
#define ARGUNUSED(x) (void)(x)
#endif

#define maxPreviewSize 500
#define previewProps 364/257

typedef QHash<int,QImage> QImageHash;
typedef QList<int> QIntList;

class ZAbstractReader;

extern ZAbstractReader *readerFactory(QObject* parent, QString filename);

QString formatSize(qint64 size);
QString escapeParam(QString param);
int compareWithNumerics(QString ref1, QString ref2);

#endif // GLOBAL_H
