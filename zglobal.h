#ifndef ZGLOBAL_H
#define ZGLOBAL_H

#include <QObject>
#include <QMap>
#include <QSettings>
#include <QPageSize>
#include "global.h"
#include "ocreditor.h"

class ZMangaModel;
class ZMainWindow;
class ZDB;
class ZTextDocumentController;

class ZGlobalPrivate;

class ZGlobal : public QObject
{
    Q_OBJECT
private:
    Q_DISABLE_COPY(ZGlobal)
    Q_DECLARE_PRIVATE_D(dptr,ZGlobal)
    QScopedPointer<ZGlobalPrivate> dptr;

    void checkSQLProblems(QWidget *parent);
    void initLanguagesList();

public:
    explicit ZGlobal(QObject *parent = nullptr);
    ~ZGlobal() override;

    void fsCheckFilesAvailability();
    void setMainWindow(ZMainWindow* wnd);
    QUrl createSearchUrl(const QString &text, const QString &engine = QString()) const;
    QStringList getLanguageCodes() const;
    QString getLanguageName(const QString &bcp47Name) const;
    qint64 getAvgFineRenderTime() const;
    ZDB *db() const;
    ZOCREditor *ocrEditor();
    ZTextDocumentController *txtController();

    // settings management
    Blitz::ScaleFilterType getUpscaleFilter() const;
    Blitz::ScaleFilterType getDownscaleFilter() const;
    Blitz::ScaleFilterType getDownscaleSearchTabFilter() const;
    Z::PDFRendering getPdfRendering() const;
    Z::DBMS getDbEngine() const;
    bool getCachePixmaps() const;
    bool getUseFineRendering() const;
    bool getFilesystemWatcher() const;
    int getCacheWidth() const;
    int getMagnifySize() const;
    int getScrollDelta() const;
    int getDetectedDelta() const;
    void setDetectedDelta(int value);
    int getScrollFactor() const;
    int getTextDocMargin() const;
    qreal getDpiX() const;
    qreal getDpiY() const;
    qreal getForceDPI() const;
    void setDPI(qreal x, qreal y);
    double getResizeBlur() const;
    double getSearchScrollFactor() const;
    QString getDefaultSearchEngine() const;
    QString getTranSourceLang() const;
    QString getTranDestLang() const;
    QString getSavedAuxOpenDir() const;
    void setSavedAuxOpenDir(const QString &value);
    QString getSavedIndexOpenDir() const;
    void setSavedIndexOpenDir(const QString &value);
    QString getSavedAuxSaveDir() const;
    void setSavedAuxSaveDir(const QString &value);
    QColor getForegroundColor() const;
    QColor getBackgroundColor() const;
    QColor getFrameColor() const;
    QColor getTextDocBkColor() const;
    QFont getIdxFont() const;
    QFont getOcrFont() const;
    QFont getTextDocFont() const;
    QString getRarCmd() const;
    void setDefaultSearchEngine(const QString &value);
    QStringList getNewlyAddedFiles() const;
    void removeFileFromNewlyAdded(const QString &filename);
    ZStrMap getBookmarks() const;
    void addBookmark(const QString& title, const QString& filename);
    ZStrMap getCtxSearchEngines() const;
    QPageSize getTextDocPageSize() const;

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
    void dbSetCoverCacheSize(int size);
    void auxMessage(const QString& msg);
    void loadSearchTabSettings(QSettings *settings);
    void saveSearchTabSettings(QSettings *settings);

};

#endif // ZGLOBAL_H
