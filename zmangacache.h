#ifndef ZMANGACACHE_H
#define ZMANGACACHE_H

#include <QtCore>
#include "global.h"
#include "zabstractreader.h"

class ZMangaCache : public QObject
{
    Q_OBJECT
protected:
    ZAbstractReader* mReader;
    QHash<int,QImage> iCache;
    int currentPage;

    void cacheDropUnusable();
    void cacheFillNearest();
    QIntList cacheGetActivePages();

public:
    explicit ZMangaCache(QObject *parent = 0);
    
signals:
    void gotPage(const QImage& page, const int& num, const QString& internalPath);
    void gotPageCount(const int& num, const int& preferred);
    void gotError(const QString& msg);
    
public slots:
    void openFile(QString filename, int preferred);
    void getPage(int num);
    void closeFile();
    
};

#endif // ZMANGACACHE_H
