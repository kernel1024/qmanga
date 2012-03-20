#include "zzipreader.h"
#include <quazip/quazipfileinfo.h>

ZZipReader::ZZipReader(QObject *parent, QString filename) :
    ZAbstractReader(parent,filename)
{
}

bool ZZipReader::openFile()
{
    if (opened)
        return false;

    QFileInfo fi(fileName);
    if (!fi.isFile() || !fi.isReadable()) {
        return false;
    }

    mainFile.setFileName(fileName);
    if (!mainFile.open(QIODevice::ReadOnly))
        return false;

    mainZFile.setIoDevice(&mainFile);
    if (!mainZFile.open(QuaZip::mdUnzip)) {
        mainFile.close();
        return false;
    }

    opened = true;
    return true;
}

void ZZipReader::closeFile()
{
    if (!opened)
        return;

    mainZFile.close();
    mainFile.close();
    opened = false;
}

int ZZipReader::getPageCountPrivate()
{
    if (!opened)
        return -1;

    int cnt = 0;
    for(bool more = mainZFile.goToFirstFile(); more; more = mainZFile.goToNextFile()) {
        QuaZipFileInfo zfi;
        mainZFile.getCurrentFileInfo(&zfi);
        if (zfi.name.endsWith('/') || zfi.name.endsWith('\\')) continue;
        cnt++;
    }

    return cnt;
}

QImage ZZipReader::loadPage(int num)
{
    QImage res;
    if (!opened)
        return res;

    QuaZipFile zf(&mainZFile);
    int idx = 0;
    for(bool more = mainZFile.goToFirstFile(); more; more = mainZFile.goToNextFile()) {
        QuaZipFileInfo zfi;
        mainZFile.getCurrentFileInfo(&zfi);
        if (zfi.name.endsWith('/') || zfi.name.endsWith('\\')) continue;

        if (idx==num) {
            QuaZipFileInfo zfi;
            mainZFile.getCurrentFileInfo(&zfi);
            if (zfi.uncompressedSize>(150*1024*1024)) {
                qDebug() << "Image file is too big (over 150Mb). Unable to load.";
                return res;
            }
            if (!zf.open(QIODevice::ReadOnly)) {
                qDebug() << "Error while opening compressed image inside archive.";
                return res;
            }
            res = QImage::fromData(zf.readAll());
            zf.close();
            return res;
        }
        idx++;
    }
    return res;
}

QImageHash ZZipReader::loadPages(QIntList nums)
{
    QImageHash hash;
    if (!opened)
        return hash;

    QuaZipFile zf(&mainZFile);
    int idx = 0;
    for(bool more = mainZFile.goToFirstFile(); more; more = mainZFile.goToNextFile()) {
        QuaZipFileInfo zfi;
        mainZFile.getCurrentFileInfo(&zfi);
        if (zfi.name.endsWith('/') || zfi.name.endsWith('\\')) continue;

        if (nums.contains(idx)) {
            QuaZipFileInfo zfi;
            mainZFile.getCurrentFileInfo(&zfi);
            if (zfi.uncompressedSize>(150*1024*1024)) {
                qDebug() << "Image file is too big (over 150Mb). Unable to load.";
            } else {
                if (!zf.open(QIODevice::ReadOnly)) {
                    qDebug() << "Error while opening compressed image inside archive.";
                } else {
                    hash[idx] = QImage::fromData(zf.readAll());
                    zf.close();
                }
            }
        }
        idx++;
    }
    return hash;
}
