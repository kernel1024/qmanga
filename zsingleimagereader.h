#ifndef ZSINGLEIMAGEREADER_H
#define ZSINGLEIMAGEREADER_H

#include <QObject>
#include <QImage>
#include "zabstractreader.h"

class ZSingleImageReader : public ZAbstractReader
{
    Q_OBJECT
public:
    ZSingleImageReader(QObject *parent, const QString &filename);
    ~ZSingleImageReader() override = default;
    bool openFile() override;
    void closeFile() override;
    QByteArray loadPage(int num) override;
    QImage loadPageImage(int num) override;
    QString getMagic() override;

private:
    QImage m_page;

    Q_DISABLE_COPY(ZSingleImageReader)

};

#endif // ZSINGLEIMAGEREADER_H
