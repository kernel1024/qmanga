#ifndef ZMANGACACHE_H
#define ZMANGACACHE_H

#include "global.h"
#include "zabstractreader.h"

class ZMangaLoader : public QObject
{
    Q_OBJECT
protected:
    ZAbstractReader* mReader;

public:
    explicit ZMangaLoader(QObject *parent = nullptr);
    ~ZMangaLoader();
    QUuid threadID;

signals:
    void gotPage(const QByteArray& page, const QImage& pageImage, const int& num,
                 const QString& internalPath, const QUuid& aThreadID);
    void gotPageCount(const int& num, const int& preferred);
    void gotError(const QString& msg);
    void closeFileRequest();
    void auxMessage(const QString& msg);
    
public slots:
    void openFile(const QString &filename, int preferred);
    void getPage(int num, bool preferImage);
    void closeFile();
    
};

#endif // ZMANGACACHE_H
