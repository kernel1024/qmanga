#ifndef ZRARREADER_H
#define ZRARREADER_H

#include "zabstractreader.h"

class ZRarReader : public ZAbstractReader
{
    Q_OBJECT
public:
    ZRarReader(QObject *parent, const QString &filename, const QString &password);
    ~ZRarReader() override = default;
    bool openFile() override;
    void closeFile() override;
    QByteArray loadPage(int num) override;
    QImage loadPageImage(int num) override;
    QString getMagic() override;
    bool isPasswordProtected() override;
    QString encryptionMethod() override;

private:
    QString m_rarExec;
    bool m_passwordProtected { false };

    Q_DISABLE_COPY(ZRarReader)

};

#endif // ZRARREADER_H
