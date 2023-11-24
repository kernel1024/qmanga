#include "abstractocr.h"

ZAbstractOCR::ZAbstractOCR(QObject *parent)
    : QObject{parent}
{
}

void ZAbstractOCR::setError(const QString &text)
{
    m_error = text;
}
