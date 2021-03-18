#include "ztextdocumentcontroller.h"

ZTextDocumentController::ZTextDocumentController(QObject *parent)
    : QObject(parent)
{
}

ZTextDocumentController::~ZTextDocumentController()
{
    QMutexLocker locker(&m_mutex);

    m_refs.clear();

    for (auto& doc : m_docs)
        doc->deleteLater();
    m_docs.clear();
}

void ZTextDocumentController::addDocument(const QString &filename, ZAbstractTextDocument *doc)
{
    QMutexLocker locker(&m_mutex);

    if (m_docs.contains(filename)) {
        auto *oldDoc = m_docs.take(filename);
        oldDoc->deleteLater();
    }
    m_docs.insert(filename,doc);
    m_refs.insert(filename,0);
}

QTextDocument *ZTextDocumentController::getDocument(const QString& filename)
{
    QMutexLocker locker(&m_mutex);

    if (!m_docs.contains(filename))
        return nullptr;

    ZAbstractTextDocument *doc = m_docs.value(filename);
    QTextDocument *res = doc->clone(nullptr);

    m_refs[filename]++;

    connect(res,&QTextDocument::destroyed,this,[this,filename](){
        QMutexLocker locker(&m_mutex);
        if (m_refs.contains(filename)) {
            m_refs[filename]--;
            if (m_refs.value(filename) == 0)
                m_refs.remove(filename);
        }
        if (!m_refs.contains(filename)) {
            if (m_docs.contains(filename)) {
                auto *doc = m_docs.take(filename);
                doc->deleteLater();
            }
        }
    });

    return res;
}

bool ZTextDocumentController::contains(const QString &filename)
{
    return m_docs.contains(filename);
}
