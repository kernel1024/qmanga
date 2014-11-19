#ifndef BOOKMARKDLG_H
#define BOOKMARKDLG_H

#include <QDialog>

namespace Ui {
class QTwoEditDlg;
}

class QTwoEditDlg : public QDialog
{
    Q_OBJECT
    
public:
    explicit QTwoEditDlg(QWidget *parent,
                         const QString &windowTitle,
                         const QString &title1, const QString &title2,
                         const QString &value1 = QString(), const QString &value2 = QString());
    ~QTwoEditDlg();
    QString getDlgEdit1();
    QString getDlgEdit2();
    void setHelpText(const QString& helpText);
    
private:
    Ui::QTwoEditDlg *ui;
};

#endif // BOOKMARKDLG_H
