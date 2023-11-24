#ifndef ZGLOBALPRIVATE_H
#define ZGLOBALPRIVATE_H

#include <QObject>
#include <QWidget>
#include <QHash>
#include <QList>
#include <QFileSystemWatcher>
#include <QMutex>
#include <QPageSize>
#include <QScopedPointer>
#include "global.h"
#include "imagescale/scalefilter.h"
#include "zglobal.h"
#include "ztextdocumentcontroller.h"

class ZMainWindow;

class ZGlobalPrivate : public QObject
{
    Q_OBJECT
public:
    explicit ZGlobalPrivate(QObject *parent = nullptr);
    ~ZGlobalPrivate() override;

    Blitz::ScaleFilterType m_upscaleFilter { Blitz::ScaleFilterType::UndefinedFilter };
    Blitz::ScaleFilterType m_downscaleFilter { Blitz::ScaleFilterType::UndefinedFilter };
    Blitz::ScaleFilterType m_downscaleSearchTabFilter { Blitz::ScaleFilterType::UndefinedFilter };
    Z::PDFRendering m_pdfRendering { Z::pdfAutodetect };
    Z::DBMS m_dbEngine { Z::dbmsSQLite };
    Z::OCREngine m_OCREngine{ Z::ocrGoogleVision };
    bool m_cachePixmaps { false };
    bool m_useFineRendering { true };
    bool m_filesystemWatcher { false };
    int m_cacheWidth { ZDefaults::cacheWidth };
    int m_magnifySize { ZDefaults::magnifySize };
    int m_scrollDelta { ZDefaults::scrollDelta };
    int m_detectedDelta { ZDefaults::scrollDelta };
    int m_scrollFactor { ZDefaults::scrollFactor };
    int m_textDocMargin { ZDefaults::textDocMargin };
    int m_maxCoverCacheSize { ZDefaults::maxCoverCacheSize };
    qreal m_dpiX { ZDefaults::standardDPI };
    qreal m_dpiY { ZDefaults::standardDPI };
    qreal m_forceDPI { ZDefaults::forceDPI };
    double m_resizeBlur { ZDefaults::resizeBlur };
    double m_searchScrollFactor { ZDefaults::searchScrollFactor };
    qint64 m_avgFineRenderTime { 0 };

    QString m_dbHost;
    QString m_dbBase;
    QString m_dbUser;
    QString m_dbPass;
    QString m_defaultSearchEngine;
    QString m_tranSourceLang;
    QString m_tranDestLang;
    QString m_savedAuxOpenDir;
    QString m_savedIndexOpenDir;
    QString m_savedAuxSaveDir;
    QString m_rarCmd;
    QString m_officeCmd;
    QString m_gcpKeyFile;
    QString m_gcpApiKey;

    QColor m_backgroundColor;
    QColor m_frameColor;
    QColor m_textDocBkColor;
    QFont m_idxFont;
    QFont m_ocrFont;
    QFont m_textDocFont;
    QPageSize m_textDocPageSize;
    QMutex m_fineRenderMutex;

    QHash<QString,QStringList> m_dirWatchList;
    QMap<QString,QString> m_langSortedBCP47List;  // names -> bcp (for sorting)
    QHash<QString,QString> m_langNamesList;       // bcp -> names
    QList<qint64> m_fineRenderTimes;
    QStringList m_newlyAddedFiles;

    ZStrMap m_bookmarks;
    ZStrMap m_ctxSearchEngines;

    QScopedPointer<ZDB, QScopedPointerDeleteLater> m_db;
    QScopedPointer<QFileSystemWatcher, QScopedPointerDeleteLater> fsWatcher;
    QScopedPointer<ZOCREditor, QScopedPointerDeleteLater> m_ocrEditor;
    QScopedPointer<QThread, QScopedPointerDeleteLater> m_threadDB;
    QScopedPointer<ZTextDocumentController, QScopedPointerDeleteLater> m_txtController;

    QPointer<ZMainWindow> m_mainWindow;

private:
    Q_DISABLE_COPY(ZGlobalPrivate)

};

#endif // ZGLOBALPRIVATE_H
