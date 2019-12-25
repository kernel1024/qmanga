#include "zglobalprivate.h"
#include "zdb.h"

ZGlobalPrivate::ZGlobalPrivate(QObject *parent)
    : QObject(parent),
      m_db(new ZDB()),
      fsWatcher(new QFileSystemWatcher(this)),
      m_ocrEditor(nullptr),
      m_threadDB(new QThread())
{

}

ZGlobalPrivate::~ZGlobalPrivate() = default;
