#ifndef BOOKMARKDLG_H
#define BOOKMARKDLG_H

#include <QDialog>

namespace Ui {
class ZTwoEditDlg;
}

class ZTwoEditDlg : public QDialog
{
    Q_OBJECT
    
public:
    explicit ZTwoEditDlg(QWidget *parent,
                         const QString &windowTitle,
                         const QString &title1, const QString &title2,
                         const QString &value1 = QString(), const QString &value2 = QString());
    ~ZTwoEditDlg() override;
    QString getDlgEdit1();
    QString getDlgEdit2();
    void setHelpText(const QString& helpText);

public Q_SLOTS:
    void setAuxText(const QString& text);
    
private:
    Ui::ZTwoEditDlg *ui;

    Q_DISABLE_COPY(ZTwoEditDlg)
};

#endif // BOOKMARKDLG_H
