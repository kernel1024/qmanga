#include <utility>

#include <QImage>
#include <QSettings>
#include <QDir>

#include "../global.h"
#include "tesseractocr.h"

#ifdef WITH_TESSERACT
tesseract::TessBaseAPI *ZTesseractOCR::m_tesseract = nullptr;
#endif
bool ZTesseractOCR::m_initFailed = false;

ZTesseractOCR::ZTesseractOCR(QObject *parent)
    : ZAbstractOCR{parent}
{
    if (m_initFailed)
        setError(QSL("Could not initialize Tesseract. Maybe language training data is not installed."));
}

void ZTesseractOCR::preinit()
{
#ifdef WITH_TESSERACT
    m_tesseract = new tesseract::TessBaseAPI();
    QByteArray tesspath = tesseractDatapath().toUtf8();

    QString lang = ZTesseractOCR::tesseractGetActiveLanguage();
    if (lang.isEmpty()
        || (m_tesseract->Init(tesspath.constData(), lang.toUtf8().constData()) != 0)) {
        m_initFailed = true;
    }
#else
    m_initFailed = true;
#endif
}

void ZTesseractOCR::processImage(const QImage &image)
{
    if (!isReady()) {
        Q_EMIT gotOCRResult(QSL("ERROR: Tesseract OCR is not initialized."));
        return;
    }

#ifdef WITH_TESSERACT
    ZTesseractOCR::m_tesseract->SetImage(Image2PIX(image));
    char *rtext = ZTesseractOCR::m_tesseract->GetUTF8Text();
    QString s = QString::fromUtf8(rtext);
    delete[] rtext;

    // vertical block transpose check
    QStringList sl = s.split(u'\n', Qt::SkipEmptyParts);
    int maxlen = 0;
    for (const auto &i : std::as_const(sl)) {
        if (i.length() > maxlen)
            maxlen = i.length();
    }
    if (maxlen < sl.count()) { // vertical kanji block, needs transpose
        QStringList sl2;
        sl2.reserve(maxlen);
        for (int i = 0; i < maxlen; i++)
            sl2 << QString(sl.count(), QChar(u' '));
        for (int i = 0; i < sl.count(); i++) {
            for (int j = 0; j < sl.at(i).length(); j++)
                sl2[maxlen - j - 1][i] = sl[i][j];
        }
        sl = sl2;
    }

    Q_EMIT gotOCRResult(sl.join(u'\n'));
#endif
}

bool ZTesseractOCR::isReady() const
{
#ifdef WITH_TESSERACT
    return ((ZTesseractOCR::m_tesseract != nullptr) && !ZTesseractOCR::m_initFailed);
#else
    return false;
#endif
}

PIX* ZTesseractOCR::Image2PIX(const QImage &qImage)
{
#ifdef WITH_TESSERACT
    PIX * pixs = nullptr;
    l_uint32 *lines = nullptr;

    QImage img = qImage.rgbSwapped();
    int width = img.width();
    int height = img.height();
    int depth = img.depth();
    int wpl = img.bytesPerLine() / 4;

    pixs = pixCreate(width, height, depth);
    pixSetWpl(pixs, wpl);
    pixSetColormap(pixs, nullptr);
    l_uint32 *datas = pixGetData(pixs);

    for (int y = 0; y < height; y++) {
        lines = datas + y * wpl; // NOLINT
        memcpy(lines,img.scanLine(y),static_cast<uint>(img.bytesPerLine()));
    }
    return pixEndianByteSwapNew(pixs);
#else
    return nullptr;
#endif
}

QString ZTesseractOCR::tesseractDatapath()
{
    QSettings settings(QSL("kernel1024"), QSL("qmanga-ocr"));
    settings.beginGroup(QSL("Main"));
#ifdef Q_OS_WIN
    QDir appDir(getApplicationDirPath());
    QString res = settings.value(QStringLiteral("datapath"),
                                 appDir.absoluteFilePath(QStringLiteral("tessdata"))).toString();
#else
    QString res = settings.value(QSL("datapath"),
                                 QSL("/usr/share/tessdata/")).toString();
#endif
    settings.endGroup();
    return res;
}

QString ZTesseractOCR::tesseractGetActiveLanguage()
{
    QSettings settings(QSL("kernel1024"), QSL("qmanga-ocr"));
    settings.beginGroup(QSL("Main"));
    QString res = settings.value(QSL("activeLanguage"),QSL("jpn")).toString();
    settings.endGroup();
    return res;
}
