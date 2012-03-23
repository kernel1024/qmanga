#ifndef ZSEARCHLOADER_H
#define ZSEARCHLOADER_H

#include <QtCore>
#include "zglobal.h"

class ZMangaModel;

class ZSearchLoader : public QThread
{
    Q_OBJECT
private:
    SQLMangaList* mList;
    ZMangaModel* model;
    QMutex* listUpdating;
    QString album, search;
public:
    explicit ZSearchLoader(QObject *parent = 0);
    void setParams(SQLMangaList* aList, QMutex* aListUpdating, ZMangaModel* aModel, QString aAlbum,
                   QString aSearch);
    void run();
    
signals:
    
public slots:
    
};

#endif // ZSEARCHLOADER_H
