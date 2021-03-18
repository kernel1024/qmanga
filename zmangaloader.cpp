#include <QDebug>
#include "zmangaloader.h"

ZMangaLoader::ZMangaLoader(QObject *parent) :
    QObject(parent)
{
}

ZMangaLoader::~ZMangaLoader()
{
    if (m_reader!=nullptr)
        closeFile();
}

void ZMangaLoader::postMessage(const QString &msg)
{
    Q_EMIT auxMessage(msg);
}

void ZMangaLoader::openFile(const QString &filename, int preferred)
{
    if (m_reader!=nullptr)
        closeFile();

    bool mimeOk = false;
    ZAbstractReader* za = zF->readerFactory(this,filename,&mimeOk,false);
    if ((za == nullptr) && (!mimeOk)) {
        Q_EMIT closeFileRequest();
        Q_EMIT gotError(tr("File format not supported."));
        return;
    }
    if ((za == nullptr) && (mimeOk)) {
        Q_EMIT closeFileRequest();
        Q_EMIT gotError(tr("File not found. Update database or restore file."));
        return;
    }
    if (!za->openFile()) {
        Q_EMIT closeFileRequest();
        Q_EMIT gotError(tr("Unable to open file."));
        za->setParent(nullptr);
        delete za;
        return;
    }
    m_reader = za;
    int pagecnt = m_reader->getPageCount();
    Q_EMIT gotPageCount(pagecnt,preferred);
}

void ZMangaLoader::getPage(int num, bool preferImage)
{
    QString ipt = m_reader->getSortEntryName(num);
    if (preferImage || !m_reader->isPageDataSupported()) {
        Q_EMIT gotPage(QByteArray(),m_reader->loadPageImage(num),num,ipt,m_threadID);
    } else {
        Q_EMIT gotPage(m_reader->loadPage(num),QImage(),num,ipt,m_threadID);
    }
}

void ZMangaLoader::closeFile()
{
    if (m_reader!=nullptr) {
        m_reader->closeFile();
        m_reader->setParent(nullptr);
        delete m_reader;
    }
    m_reader = nullptr;
}
