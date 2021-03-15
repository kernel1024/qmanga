#ifndef ZZIPREADER_H
#define ZZIPREADER_H

#include <zip.h>
#include "zabstractreader.h"

class ZZipReader : public ZAbstractReader
{
    Q_OBJECT
public:
    ZZipReader(QObject *parent, const QString &filename);
    ~ZZipReader() override = default;
    bool openFile() override;
    void closeFile() override;
    QByteArray loadPage(int num) override;
    QImage loadPageImage(int num) override;
    QString getMagic() override;

private:
    zip_t* m_zFile;
    QHash <int,int> m_sizes;

    Q_DISABLE_COPY(ZZipReader)
};

#endif // ZZIPREADER_H
