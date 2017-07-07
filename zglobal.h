#ifndef ZGLOBAL_H
#define ZGLOBAL_H

#include <QFileSystemWatcher>
#include <QHash>
#include <QMap>
#include <QUrl>
#include "global.h"
#include "ocreditor.h"

class ZMangaModel;
class ZDB;

typedef QMap<QString, QString> ZStrMap;

class ZGlobal : public QObject
{
    Q_OBJECT
private:
    QThread* threadDB;
    QString dbHost, dbBase, dbUser, dbPass;
    QHash<QString,QStringList> dirWatchList;

    void checkSQLProblems(QWidget *parent);

public:
    ZDB* db;
    ZOCREditor* ocrEditor;
    QFileSystemWatcher* fsWatcher;
    QStringList newlyAddedFiles;

    explicit ZGlobal(QObject *parent = 0);
    ~ZGlobal();

    ZStrMap bookmarks;

    ZStrMap ctxSearchEngines;
    QString defaultSearchEngine;

    Blitz::ScaleFilterType downscaleFilter, upscaleFilter;
    Z::Ordering defaultOrdering;
    Z::PDFRendering pdfRendering;
    Z::DBMS dbEngine;
    int cacheWidth;
    int magnifySize;
    qreal resizeBlur;
    QString savedAuxOpenDir, savedIndexOpenDir, savedAuxSaveDir;
    QColor backgroundColor, frameColor;
    QFont idxFont, ocrFont;
    QString rarCmd;
    int preferredWidth;
    int scrollDelta;
    int detectedDelta;
    int scrollFactor;

    bool cachePixmaps;
    bool useFineRendering;
    bool filesystemWatcher;

    qreal dpiX, dpiY;
    qreal forceDPI;

    QColor foregroundColor();

    void fsCheckFilesAvailability();
    QUrl createSearchUrl(const QString &text, const QString &engine = QString());

public slots:
    void settingsDlg();
    void loadSettings();
    void saveSettings();
    void updateWatchDirList(const QStringList & watchDirs);
    void directoryChanged(const QString & dir);
    void resetPreferredWidth();
    void dbCheckComplete();

signals:
    void dbSetCredentials(const QString& host, const QString& base,
                          const QString& user, const QString& password);
    void dbCheckBase();
    void dbCheckEmptyAlbums();
    void dbRescanIndexedDirs();
    void fsFilesAdded();
    void dbSetDynAlbums(const ZStrMap& albums);
    void dbSetIgnoredFiles(const QStringList& files);

};

class ZFileEntry {
public:
    QString name;
    int idx;
    ZFileEntry();
    ZFileEntry(const ZFileEntry& other);
    ZFileEntry(QString aName, int aIdx);
    ZFileEntry &operator=(const ZFileEntry& other);
    bool operator==(const ZFileEntry& ref) const;
    bool operator!=(const ZFileEntry& ref) const;
    bool operator<(const ZFileEntry& ref) const;
    bool operator>(const ZFileEntry& ref) const;
};


extern ZGlobal* zg;

#endif // ZGLOBAL_H
