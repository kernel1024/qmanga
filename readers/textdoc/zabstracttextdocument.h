#ifndef ZABSTRACTTEXTDOCUMENT_H
#define ZABSTRACTTEXTDOCUMENT_H

#include <QTextDocument>

class ZAbstractTextDocument : public QTextDocument
{
    Q_OBJECT
    Q_DISABLE_COPY(ZAbstractTextDocument)

public:
    explicit ZAbstractTextDocument(QObject* parent);
    ~ZAbstractTextDocument() override;

    static QSize defaultPageSizePixels();

    virtual bool isValid() = 0;

protected:
    int maxContentHeight() const;
    int maxContentWidth() const;

private:
    int m_pageMargin;

};

#endif // ZABSTRACTTEXTDOCUMENT_H
