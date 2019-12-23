#ifndef ZGLOBAL_H
#define ZGLOBAL_H

#include <QFileSystemWatcher>
#include <QHash>
#include <QMap>
#include <QUrl>
#include <QList>
#include <QMutex>
#include "global.h"
#include "ocreditor.h"

class ZMangaModel;
class ZDB;

using ZStrMap = QMap<QString, QString>;

class ZGlobal : public QObject
{
    Q_OBJECT
private:
    Q_DISABLE_COPY(ZGlobal)

    QThread* threadDB;
    QString dbHost, dbBase, dbUser, dbPass;
    QHash<QString,QStringList> dirWatchList;
    QMap<QString,QString> langSortedBCP47List;  // names -> bcp (for sorting)
    QHash<QString,QString> langNamesList;       // bcp -> names
    QList<qint64> fineRenderTimes;
    QMutex fineRenderMutex;
    qint64 avgFineRenderTime { 0 };

    void checkSQLProblems(QWidget *parent);
    void initLanguagesList();

public:
    ZDB* db;
    ZOCREditor* ocrEditor { nullptr };
    QFileSystemWatcher* fsWatcher;
    QStringList newlyAddedFiles;

    explicit ZGlobal(QObject *parent = nullptr);
    ~ZGlobal() override;

    Blitz::ScaleFilterType downscaleFilter { Blitz::ScaleFilterType::UndefinedFilter };
    Blitz::ScaleFilterType upscaleFilter { Blitz::ScaleFilterType::UndefinedFilter };
    Z::PDFRendering pdfRendering { Z::Autodetect };
    Z::DBMS dbEngine { Z::SQLite };
    bool cachePixmaps { false };
    bool useFineRendering { true };
    bool filesystemWatcher { false };
    int cacheWidth { 6 };
    int magnifySize { 100 };
    int scrollDelta { 120 };
    int detectedDelta { 120 };
    int scrollFactor { 3 };
    qreal dpiX { 75.0 };
    qreal dpiY { 75.0 };
    qreal forceDPI { -1.0 };
    double resizeBlur { 1.0 };
    double searchScrollFactor { 0.1 };

    ZStrMap bookmarks;
    ZStrMap ctxSearchEngines;
    QString defaultSearchEngine;
    QString tranSourceLang, tranDestLang;
    QString savedAuxOpenDir, savedIndexOpenDir, savedAuxSaveDir;
    QColor backgroundColor, frameColor;
    QFont idxFont, ocrFont;
    QString rarCmd;

    QColor foregroundColor();

    void fsCheckFilesAvailability();
    QUrl createSearchUrl(const QString &text, const QString &engine = QString());

    QStringList getLanguageCodes() const;
    QString getLanguageName(const QString &bcp47Name);
    qint64 getAvgFineRenderTime();

public Q_SLOTS:
    void settingsDlg();
    void loadSettings();
    void saveSettings();
    void updateWatchDirList(const QStringList & watchDirs);
    void directoryChanged(const QString & dir);
    void dbCheckComplete();
    void addFineRenderTime(qint64 msec);

Q_SIGNALS:
    void dbSetCredentials(const QString& host, const QString& base,
                          const QString& user, const QString& password);
    void dbCheckBase();
    void dbRescanIndexedDirs();
    void fsFilesAdded();
    void dbSetDynAlbums(const ZStrMap& albums);
    void dbSetIgnoredFiles(const QStringList& files);
    void auxMessage(const QString& msg);
    void loadSearchTabSettings(QSettings *settings);
    void saveSearchTabSettings(QSettings *settings);

};

class ZFileEntry {
public:
    QString name;
    int idx { -1 };
    ZFileEntry() = default;
    ~ZFileEntry() = default;
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
