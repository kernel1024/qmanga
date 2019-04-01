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

using ZStrMap = QMap<QString, QString>;

class ZGlobal : public QObject
{
    Q_OBJECT
private:
    QThread* threadDB;
    QString dbHost, dbBase, dbUser, dbPass;
    QHash<QString,QStringList> dirWatchList;
    QMap<QString,QString> langSortedBCP47List;  // names -> bcp (for sorting)
    QHash<QString,QString> langNamesList;       // bcp -> names

    void checkSQLProblems(QWidget *parent);
    void initLanguagesList();

public:
    ZDB* db;
    ZOCREditor* ocrEditor;
    QFileSystemWatcher* fsWatcher;
    QStringList newlyAddedFiles;

    explicit ZGlobal(QObject *parent = nullptr);
    ~ZGlobal();

    ZStrMap bookmarks;

    ZStrMap ctxSearchEngines;
    QString defaultSearchEngine;

    QString tranSourceLang, tranDestLang;

    Blitz::ScaleFilterType downscaleFilter, upscaleFilter;
    Z::Ordering defaultOrdering;
    Qt::SortOrder defaultOrderingDirection;
    Z::PDFRendering pdfRendering;
    Z::DBMS dbEngine;
    int cacheWidth;
    int magnifySize;
    double resizeBlur;
    QString savedAuxOpenDir, savedIndexOpenDir, savedAuxSaveDir;
    QColor backgroundColor, frameColor;
    QFont idxFont, ocrFont;
    QString rarCmd;
    int scrollDelta;
    int detectedDelta;
    int scrollFactor;
    double searchScrollFactor;

    bool cachePixmaps;
    bool useFineRendering;
    bool filesystemWatcher;

    qreal dpiX, dpiY;
    qreal forceDPI;

    QColor foregroundColor();

    void fsCheckFilesAvailability();
    QUrl createSearchUrl(const QString &text, const QString &engine = QString());

    QStringList getLanguageCodes() const;
    QString getLanguageName(const QString &bcp47Name);

public slots:
    void settingsDlg();
    void loadSettings();
    void saveSettings();
    void updateWatchDirList(const QStringList & watchDirs);
    void directoryChanged(const QString & dir);
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
    ZFileEntry(const QString &aName, int aIdx);
    ZFileEntry &operator=(const ZFileEntry& other) = default;
    bool operator==(const ZFileEntry& ref) const;
    bool operator!=(const ZFileEntry& ref) const;
    bool operator<(const ZFileEntry& ref) const;
    bool operator>(const ZFileEntry& ref) const;
};


extern ZGlobal* zg;

#endif // ZGLOBAL_H
