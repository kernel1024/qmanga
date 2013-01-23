#ifndef ZGLOBAL_H
#define ZGLOBAL_H

#include <QtCore>

#include "global.h"
#include "ocreditor.h"

class ZMangaModel;
class ZDB;

class ZGlobal : public QObject
{
    Q_OBJECT
protected:
    QThread* threadDB;
    QString dbBase, dbUser, dbPass;
public:
    ZDB* db;
    ZOCREditor* ocrEditor;

    explicit ZGlobal(QObject *parent = 0);
    ~ZGlobal();

    QMap<QString, QString> bookmarks;

    Z::ResizeFilter resizeFilter;
    int cacheWidth;
    int magnifySize;
    QString savedAuxOpenDir, savedIndexOpenDir;
    QColor backgroundColor;
    QFont idxFont;

    int pdfRenderWidth;
    bool cachePixmaps;
    bool useFineRendering;

    QColor foregroundColor();
    
public slots:
    void settingsDlg();
    void loadSettings();
    void saveSettings();

signals:
    void dbSetCredentials(const QString& base, const QString& user, const QString& password);
    void dbCheckBase();
    void dbCheckEmptyAlbums();

};

class ZFileEntry {
public:
    QString name;
    int idx;
    ZFileEntry();
    ZFileEntry(QString aName, int aIdx);
    ZFileEntry &operator=(const ZFileEntry& other);
    bool operator==(const ZFileEntry& ref) const;
    bool operator!=(const ZFileEntry& ref) const;
    bool operator<(const ZFileEntry& ref) const;
    bool operator>(const ZFileEntry& ref) const;
};


extern ZGlobal* zg;

#endif // ZGLOBAL_H
