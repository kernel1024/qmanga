#ifndef ZIMAGESDIRREADER_H
#define ZIMAGESDIRREADER_H

#include "zglobal.h"
#include "zabstractreader.h"

class ZImagesDirReader : public ZAbstractReader
{
    Q_OBJECT
public:
    ZImagesDirReader(QObject *parent, const QString &filename);
    ~ZImagesDirReader() override = default;
    bool openFile() override;
    QByteArray loadPage(int num) override;
    QImage loadPageImage(int num) override;
    QString getMagic() override;

private:
    Q_DISABLE_COPY(ZImagesDirReader)

};

#endif // ZIMAGESDIRREADER_H
