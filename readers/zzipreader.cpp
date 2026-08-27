#include <QDebug>
#include "zzipreader.h"

ZZipReader::ZZipReader(QObject *parent, const QString &filename, const QString &password) :
    ZAbstractReader(parent,filename,password)
{
    m_zFile = nullptr;
}

bool ZZipReader::openFile()
{
    if (isOpened())
        return false;

    m_sizes.clear();
    m_passwordProtected = false;
    m_encryptionMethod.clear();

    QString fileName = getFileName();
    QFileInfo fi(fileName);
    if (!fi.isFile() || !fi.isReadable()) {
        return false;
    }

    m_zFile = zip_open(fileName.toUtf8().constData(),ZIP_RDONLY,nullptr);
    if (!m_zFile) {
        return false;
    }

    QString password = accessPassword();
    if (!password.isEmpty()) {
        zip_set_default_password(m_zFile,password.toUtf8().constData());
    }

    qint64 count = zip_get_num_entries(m_zFile,0);
    for (qint64 idx = 0; idx < count; idx++) {
        zip_stat_t stat;
        if (zip_stat_index(m_zFile,static_cast<uint>(idx),
                           ZIP_STAT_SIZE | ZIP_STAT_NAME | ZIP_STAT_ENCRYPTION_METHOD,&stat)<0) {
            qDebug() << "Unable to get file stat for index " << idx;
            continue;
        }
        QString fname = QString::fromUtf8(stat.name);
        if (fname.endsWith(u'/') || fname.endsWith(u'\\')) continue;
        QFileInfo fi(fname);
        if (!ZGenericFuncs::supportedImg().contains(fi.suffix(),Qt::CaseInsensitive)) continue;
        addSortEntry(fname,static_cast<int>(idx));

        m_sizes[static_cast<int>(idx)] = static_cast<int>(stat.size);

        if ((stat.encryption_method != ZIP_EM_NONE) && (!m_passwordProtected)) {
            m_passwordProtected = true;
            qDebug() << "The file is password-protected.";
            switch (stat.encryption_method) {
                case ZIP_EM_TRAD_PKWARE: m_encryptionMethod = QSL("PKWARE"); break;
                case ZIP_EM_AES_128: m_encryptionMethod = QSL("AES-128"); break;
                case ZIP_EM_AES_192: m_encryptionMethod = QSL("AES-192"); break;
                case ZIP_EM_AES_256: m_encryptionMethod = QSL("AES-256"); break;
                case ZIP_EM_UNKNOWN: m_encryptionMethod = QSL("UNKNOWN"); break;
                default: m_encryptionMethod = QSL("OTHER"); break;
            }
        }
    }

    performListSort();

    setOpenFileSuccess();
    return true;
}

void ZZipReader::closeFile()
{
    if (!isOpened())
        return;

    if (m_zFile!=nullptr)
        zip_close(m_zFile);
    m_sizes.clear();
    ZAbstractReader::closeFile();
}

QByteArray ZZipReader::loadPage(int num)
{
    QByteArray res;
    if (!isOpened())
        return res;

    int znum = getSortEntryIdx(num);
    if (znum<0) {
        setErrorMessage(QSL("Incorrect page number"));
        return res;
    }

    int size = m_sizes.value(znum);
    if (size>ZDefaults::maxImageFileSize) {
        setErrorMessage(QSL("Image file is too big (over 150Mb). Unable to load."));
        return res;
    }

    res.resize(size);

    zip_file_t* file = nullptr;

    auto formatZipError = [this,&file](){
        zip_error_t *z_err = zip_get_error(m_zFile);
        if (file != nullptr) {
            z_err = zip_file_get_error(file);
        }
        int err_code = zip_error_code_zip(z_err);

        if (err_code == ZIP_ER_WRONGPASSWD) {
            setErrorMessage(QSL("ZIP archive access denied: Incorrect password provided."));
        } else {
            setErrorMessage(QSL("ZIP read failed due to error: %1").arg(zip_error_strerror(z_err)));
        }
    };

    file = zip_fopen_index(m_zFile,static_cast<uint>(znum),0);
    if (file != nullptr) {
        qint64 sz = zip_fread(file,res.data(),static_cast<uint>(size));
        zip_fclose(file);

        if (sz < 0) {
            res.clear();
            formatZipError();
            return res;
        }
        if (sz<size)
            res.truncate(static_cast<int>(sz));
    } else {
        formatZipError();
    }

    return res;
}

QImage ZZipReader::loadPageImage(int num)
{
    const QByteArray buf = loadPage(num);
    if (buf.isEmpty()) return QImage();

    QImage img;
    img.loadFromData(buf);
    return img;
}

QString ZZipReader::getMagic()
{
    return QSL("ZIP");
}

bool ZZipReader::isPasswordProtected()
{
    return m_passwordProtected;
}

QString ZZipReader::encryptionMethod()
{
    return m_encryptionMethod;
}
