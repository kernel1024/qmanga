#include "zsearchloader.h"
#include "zmangamodel.h"

ZSearchLoader::ZSearchLoader(QObject *parent) :
    QThread(parent)
{
    mList = NULL;
    listUpdating = NULL;
    model = NULL;
    album = QString();
    search = QString();
}

void ZSearchLoader::setParams(SQLMangaList *aList, QMutex *aListUpdating, ZMangaModel *aModel, QString aAlbum, QString aSearch)
{
    mList = aList;
    listUpdating = aListUpdating;
    model = aModel;
    album = aAlbum;
    search = aSearch;
}

void ZSearchLoader::run()
{
    listUpdating->lock();
    int cnt = mList->count()-1;
    listUpdating->unlock();

    QMetaObject::invokeMethod(model,"rowsAboutToDeleted",Qt::QueuedConnection,
                              Q_ARG(int,0),
                              Q_ARG(int,cnt));
    listUpdating->lock();
    mList->clear();
    listUpdating->unlock();
    QMetaObject::invokeMethod(model,"rowsDeleted",Qt::QueuedConnection);

    zGlobal->sqlGetFiles(album,search,mList,listUpdating,model);
}
