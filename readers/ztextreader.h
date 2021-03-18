#ifndef ZTEXTREADER_H
#define ZTEXTREADER_H

#include <QObject>
#include <QTextDocument>
#include <QPointer>
#include "zabstractreader.h"

class ZTextReader : public ZAbstractReader
{
    Q_OBJECT
    Q_DISABLE_COPY(ZTextReader)
private:
    QPointer<QTextDocument> m_doc;
    QFont m_defaultFont;

public:
    ZTextReader(QObject *parent, const QString &filename);
    ~ZTextReader() override;

    static bool preloadFile(const QString& filename, bool preserveDocument);

    QFont getDefaultFont() const;

    bool openFile() override;
    void closeFile() override;
    QByteArray loadPage(int num) override;
    QImage loadPageImage(int num) override;
    bool isPageDataSupported() override;
    QString getMagic() override;

};

#endif // ZTEXTREADER_H
