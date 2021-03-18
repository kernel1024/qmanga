#ifndef ZTEXTDOCUMENTCONTROLLER_H
#define ZTEXTDOCUMENTCONTROLLER_H

#include <QObject>
#include <QMutex>
#include "readers/textdoc/zabstracttextdocument.h"
#include "readers/ztextreader.h"

class ZTextDocumentController : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ZTextDocumentController)

private:
    QMutex m_mutex;
    QHash<QString, ZAbstractTextDocument*> m_docs;
    QHash<QString, int> m_refs;

public:
    explicit ZTextDocumentController(QObject *parent = nullptr);
    ~ZTextDocumentController() override;

    void addDocument(const QString& filename, ZAbstractTextDocument* doc);
    QTextDocument *getDocument(const QString &filename);
    bool contains(const QString& filename);

};

#endif // ZTEXTDOCUMENTCONTROLLER_H
