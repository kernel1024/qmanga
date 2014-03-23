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
    explicit ZExportDialog(QWidget *parent = 0);
    ~ZExportDialog();
    void setPagesMaximum(const int pagesMaximum);
    QString getExportDir();
    QString getImageFormat();
    int getPagesCount();
    int getImageQuality();

public slots:
    void dirSelectBtn();

private:
    Ui::ZExportDialog *ui;
};

#endif // ZEXPORTDIALOG_H
