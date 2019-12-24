#include "zglobalprivate.h"
#include "zdb.h"

ZGlobalPrivate::ZGlobalPrivate(QObject *parent, QWidget *parentWidget)
    : QObject(parent),
      m_db(new ZDB()),
      fsWatcher(new QFileSystemWatcher(this)),
      m_ocrEditor(new ZOCREditor(parentWidget)),
      m_threadDB(new QThread())
{

}

ZGlobalPrivate::~ZGlobalPrivate() = default;
