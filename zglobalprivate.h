#ifndef ZGLOBALPRIVATE_H
#define ZGLOBALPRIVATE_H

#include <QObject>
#include <QWidget>
#include <QHash>
#include <QList>
#include <QFileSystemWatcher>
#include <QMutex>
#include <QScopedPointer>
#include "global.h"
#include "scalefilter.h"
#include "zglobal.h"

class ZGlobalPrivate : public QObject
{
    Q_OBJECT
public:
    ZGlobalPrivate(QObject *parent, QWidget* parentWidget);
    ~ZGlobalPrivate() override;

    Blitz::ScaleFilterType m_downscaleFilter { Blitz::ScaleFilterType::UndefinedFilter };
    Blitz::ScaleFilterType m_upscaleFilter { Blitz::ScaleFilterType::UndefinedFilter };
    Z::PDFRendering m_pdfRendering { Z::pdfAutodetect };
    Z::DBMS m_dbEngine { Z::dbmsSQLite };
    bool m_cachePixmaps { false };
    bool m_useFineRendering { true };
    bool m_filesystemWatcher { false };
    int m_cacheWidth { ZDefaults::cacheWidth };
    int m_magnifySize { ZDefaults::magnifySize };
    int m_scrollDelta { ZDefaults::scrollDelta };
    int m_detectedDelta { ZDefaults::scrollDelta };
    int m_scrollFactor { ZDefaults::scrollFactor };
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

    QColor m_backgroundColor;
    QColor m_frameColor;
    QFont m_idxFont;
    QFont m_ocrFont;
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

private:
    Q_DISABLE_COPY(ZGlobalPrivate)

};

#endif // ZGLOBALPRIVATE_H
