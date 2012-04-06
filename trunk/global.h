#ifndef GLOBAL_H
#define GLOBAL_H

#include <QtCore>
#include <QtGui>
#include <magic.h>

#ifdef QB_KDEDIALOGS
#include <kfiledialog.h>
#include <kurl.h>
#endif

#ifndef ARGUNUSED
#define ARGUNUSED(x) (void)(x)
#endif

#define maxPreviewSize 500
#define previewProps 364/257

typedef QHash<int,QImage> QImageHash;
typedef QList<int> QIntList;

namespace Z {

enum ResizeFilter {
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

enum Ordering {
    Unordered = 0,
    FileName = 1,
    FileDate = 2,
    AddingDate = 3,
    Album = 4,
    PagesCount = 5
};

}

class ZAbstractReader;

extern ZAbstractReader *readerFactory(QObject* parent, QString filename);

QString formatSize(qint64 size);
QString escapeParam(QString param);
int compareWithNumerics(QString ref1, QString ref2);

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

QString detectMIME(QString filename);
QString detectMIME(QByteArray buf);
QPixmap resizeImage(QPixmap src, QSize targetSize,
                    bool forceFilter = false, Z::ResizeFilter filter = Z::Lanczos);

#endif // GLOBAL_H
