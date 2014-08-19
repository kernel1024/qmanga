#ifndef ZGLOBAL_H
#define ZGLOBAL_H

#include <QFileSystemWatcher>
#include <QHash>
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
    QHash<QString,QStringList> dirWatchList;
public:
    ZDB* db;
    ZOCREditor* ocrEditor;
    QFileSystemWatcher* fsWatcher;
    QStringList newlyAddedFiles;

    explicit ZGlobal(QObject *parent = 0);
    ~ZGlobal();

    QMap<QString, QString> bookmarks;

    Z::ResizeFilter resizeFilter;
    Z::Ordering defaultOrdering;
    int cacheWidth;
    int magnifySize;
    QString savedAuxOpenDir, savedIndexOpenDir;
    QColor backgroundColor, frameColor;
    QFont idxFont;
    int preferredWidth;
    int scrollDelta;
    int detectedDelta;

    bool cachePixmaps;
    bool useFineRendering;
    bool filesystemWatcher;

    QColor foregroundColor();

    void fsCheckFilesAvailability();

public slots:
    void settingsDlg();
    void loadSettings();
    void saveSettings();
    void updateWatchDirList(const QStringList & watchDirs);
    void directoryChanged(const QString & dir);
    void resetPreferredWidth();

signals:
    void dbSetCredentials(const QString& base, const QString& user, const QString& password);
    void dbCheckBase();
    void dbCheckEmptyAlbums();
    void dbRescanIndexedDirs();
    void fsFilesAdded();

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
