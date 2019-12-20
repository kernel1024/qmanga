#ifndef ZEXPORTDIALOG_H
#define ZEXPORTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>

namespace Ui {
class ZExportDialog;
}

class ZExportDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ZExportDialog(QWidget *parent = nullptr);
    ~ZExportDialog() override;
    void setPages(int currentPage, int pagesMaximum);
    QString getExportDir();
    QString getImageFormat();
    int getPagesCount();
    int getImageQuality();
    void setExportDir(const QString& dir);

public Q_SLOTS:
    void dirSelectBtn();
    void updateRange(int value);

private:
    Ui::ZExportDialog *ui;
    int m_currentPage;

    Q_DISABLE_COPY(ZExportDialog)
};

#endif // ZEXPORTDIALOG_H
