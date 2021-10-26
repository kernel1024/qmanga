#include <QTextFrame>
#include <QTextFrameFormat>
#include "ztxtdocument.h"
#include "global.h"

ZTxtDocument::ZTxtDocument(QObject *parent, const QString &fileName)
    : ZAbstractTextDocument(parent)
{
    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly)) {
        m_loadedOk = false;
        return;
    }

    const QString text = ZGenericFuncs::detectDecodeToUnicode(f.readAll());
    f.close();

    setPlainText(text);

    m_loadedOk = true;
}

ZTxtDocument::~ZTxtDocument() = default;

bool ZTxtDocument::isValid()
{
    return m_loadedOk;
}
