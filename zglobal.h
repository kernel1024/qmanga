#ifndef ZGLOBAL_H
#define ZGLOBAL_H

#include <QtCore>
#include <QtGui>
#include <QtSql>
#include <magic.h>

#ifdef QB_KDEDIALOGS
#include <kfiledialog.h>
#include <kurl.h>
#endif

#include "global.h"

class SQLMangaEntry {
public:
    int dbid;
    QString name;
    QString filename;
    QString album;
    QImage cover;
    int pagesCount;
    qint64 fileSize;
    QString fileMagic;
    QDateTime fileDT;
    QDateTime addingDT;
    SQLMangaEntry();
    SQLMangaEntry(int aDbid);
    SQLMangaEntry(const QString& aName, const QString& aFilename, const QString& aAlbum,
                  const QImage &aCover, const int aPagesCount, const qint64 aFileSize,
                  const QString& aFileMagic, const QDateTime& aFileDT, const QDateTime& aAddingDT, int aDbid);
    SQLMangaEntry &operator=(const SQLMangaEntry& other);
    bool operator==(const SQLMangaEntry& ref) const;
    bool operator!=(const SQLMangaEntry& ref) const;
};

typedef QList<SQLMangaEntry> SQLMangaList;

class ZMangaModel;

class ZGlobal : public QObject
{
    Q_OBJECT
public:
    enum ZResizeFilter {
        Nearest = 0,
        Bilinear = 1,
        Lanczos = 2,
        Gaussian = 3,
        Lanczos2 = 4,
        Cubic = 5,
        Sinc = 6,
        Triangle = 7,
        Mitchell = 8
    };

    enum ZOrdering {
        Unordered = 0,
        FileName = 1,
        FileDate = 2,
        AddingDate = 3,
        Album = 4,
        PagesCount = 5
    };

    explicit ZGlobal(QObject *parent = 0);

    QMap<QString, QString> bookmarks;

    ZResizeFilter resizeFilter;
    int cacheWidth;
    int magnifySize;
    QString savedAuxOpenDir, savedIndexOpenDir;
    QColor backgroundColor;
    QFont idxFont;
    QString dbBase, dbUser, dbPass;

    int dpiX, dpiY;

    void loadSettings();
    void saveSettings();

    QString detectMIME(QString filename);
    QString detectMIME(QByteArray buf);
    QPixmap resizeImage(QPixmap src, QSize targetSize,
                        bool forceFilter = false, ZResizeFilter filter = Lanczos);
    QColor foregroundColor();

    bool sqlCheckBase();
    bool sqlCreateTables();
    QSqlDatabase sqlOpenBase();
    void sqlCloseBase(QSqlDatabase& db);
    QStringList sqlGetAlbums();
    void sqlGetFiles(QString album, QString search, SQLMangaList* mList,
                     QMutex* listUpdating, ZMangaModel *model, ZOrdering order, bool reverseOrder);
    void sqlAddFiles(QStringList aFiles, QString album);
    void sqlDelFiles(QIntList dbids);
    void sqlDelEmptyAlbums();
    void sqlRenameAlbum(QString oldName, QString newName);


    QString getKDEFilters(const QString & qfilter);
    QString getOpenFileNameD ( QWidget * parent = 0,
                               const QString & caption = QString(),
                               const QString & dir = QString(),
                               const QString & filter = QString(),
                               QString * selectedFilter = 0,
                               QFileDialog::Options options = 0 );
    QStringList getOpenFileNamesD ( QWidget * parent = 0,
                                    const QString & caption = QString(),
                                    const QString & dir = QString(),
                                    const QString & filter = QString(),
                                    QString * selectedFilter = 0,
                                    QFileDialog::Options options = 0 );
    QString getSaveFileNameD ( QWidget * parent = 0,
                               const QString & caption = QString(),
                               const QString & dir = QString(),
                               const QString & filter = QString(),
                               QString * selectedFilter = 0,
                               QFileDialog::Options options = 0 );
    QString	getExistingDirectoryD ( QWidget * parent = 0,
                                    const QString & caption = QString(),
                                    const QString & dir = QString(),
                                    QFileDialog::Options options = QFileDialog::ShowDirsOnly );
    
public slots:
    void settingsDlg();

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


extern ZGlobal* zGlobal;

#endif // ZGLOBAL_H
