#include "zmangaloader.h"

ZMangaLoader::ZMangaLoader(QObject *parent) :
    QObject(parent)
{
    mReader = NULL;
}

void ZMangaLoader::openFile(QString filename, int preferred)
{
    if (mReader!=NULL)
        closeFile();

    bool mimeOk = false;
    ZAbstractReader* za = readerFactory(this,filename,&mimeOk,false);
    if ((za == NULL) && (!mimeOk)) {
        emit closeFileRequest();
        emit gotError(tr("File format not supported."));
        return;
    } else if ((za == NULL) && (mimeOk)) {
        emit closeFileRequest();
        emit gotError(tr("File not found. Update database or restore file."));
        return;
    }
    if (!za->openFile()) {
        emit closeFileRequest();
        emit gotError(tr("Unable to open file."));
        za->setParent(NULL);
        delete za;
        return;
    }
    mReader = za;
    int pagecnt = mReader->getPageCount();
    emit gotPageCount(pagecnt,preferred);
}

void ZMangaLoader::getPage(int num)
{
    QString ipt = mReader->getInternalPath(num);
    QByteArray img = mReader->loadPage(num);

    emit gotPage(img,num,ipt,threadID);
}

void ZMangaLoader::closeFile()
{
    if (mReader!=NULL) {
        mReader->closeFile();
        mReader->setParent(NULL);
        delete mReader;
    }
    mReader = NULL;
}
