#ifndef ZTXTDOCUMENT_H
#define ZTXTDOCUMENT_H

#include "zabstracttextdocument.h"

class ZTxtDocument : public ZAbstractTextDocument
{
    Q_OBJECT
    Q_DISABLE_COPY(ZTxtDocument)

private:
    bool m_loadedOk { false };

public:
    ZTxtDocument(QObject* parent, const QString &fileName);
    ~ZTxtDocument() override;

    bool isValid() override;
};

#endif // ZTXTDOCUMENT_H
