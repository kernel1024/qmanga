#ifndef ZABSTRACTOCR_H
#define ZABSTRACTOCR_H

#include <QObject>
#include <QImage>
#include <QString>

class ZAbstractOCR : public QObject
{
    Q_OBJECT
public:
    explicit ZAbstractOCR(QObject *parent = nullptr);

    virtual bool isReady() const = 0;
    virtual void processImage(const QImage &image) = 0;

    QString lastError() const { return m_error; }

Q_SIGNALS:
    void gotOCRResult(const QString &text);

protected:
    void setError(const QString &text);

private:
    QString m_error;

};

#endif // ZABSTRACTOCR_H
