#ifndef ZGLOBAL_H
#define ZGLOBAL_H

#include <QtCore>
#include <QtGui>
#include <magic.h>

#ifdef QB_KDEDIALOGS
#include <kfiledialog.h>
#include <kurl.h>
#endif

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

typedef QHash<int,QImage> QImageHash;
typedef QList<int> QIntList;

class ZGlobal : public QObject
{
    Q_OBJECT
public:
    enum ZResizeFilter {
        Nearest = 0,
        Bilinear = 1,
        Lanczos = 2
    };
    explicit ZGlobal(QObject *parent = 0);

    int cacheWidth;
    ZResizeFilter resizeFilter;

    void loadSettings();
    void saveSettings();

    QString mysqlUser;
    QString mysqlPassword;

    QString savedAuxOpenDir;
    
    QString detectMIME(QString filename);
    QString detectMIME(QByteArray buf);
    QPixmap resizeImage(QPixmap src, QSize targetSize);

#ifdef QB_KDEDIALOGS
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
#endif
signals:
    
public slots:
    
};

extern ZGlobal* zGlobal;

#endif // ZGLOBAL_H
