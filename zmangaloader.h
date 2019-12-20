#ifndef ZMANGACACHE_H
#define ZMANGACACHE_H

#include "global.h"
#include "zabstractreader.h"

class ZLoaderHelper;

class ZMangaLoader : public QObject
{
    Q_OBJECT
public:
    explicit ZMangaLoader(QObject *parent = nullptr);
    ~ZMangaLoader() override;
    void postMessage(const QString& msg);

Q_SIGNALS:
    void gotPage(const QByteArray& page, const QImage& pageImage, int num,
                 const QString& internalPath, const QUuid& aThreadID);
    void gotPageCount(int num, int preferred);
    void gotError(const QString& msg);
    void closeFileRequest();
    void auxMessage(const QString& msg);
    
public Q_SLOTS:
    void openFile(const QString &filename, int preferred);
    void getPage(int num, bool preferImage);
    void closeFile();

private:
    ZAbstractReader* m_reader { nullptr };
    QUuid m_threadID;

    Q_DISABLE_COPY(ZMangaLoader)

    friend class ZLoaderHelper;
};

#endif // ZMANGACACHE_H
