#include "bookmarkdlg.h"
#include "ui_bookmarkdlg.h"

QTwoEditDlg::QTwoEditDlg(QWidget *parent, const QString& windowTitle,
                         const QString &title1, const QString &title2,
                         const QString &value1, const QString &value2) :
    QDialog(parent),
    ui(new Ui::QTwoEditDlg)
{
    ui->setupUi(this);
    setWindowTitle(windowTitle);
    ui->label1->setText(title1);
    ui->label2->setText(title2);
    ui->lineEdit1->setText(value1);
    ui->lineEdit2->setText(value2);
    ui->labelHelp->clear();
}

QTwoEditDlg::~QTwoEditDlg()
{
    delete ui;
}

QString QTwoEditDlg::getDlgEdit1()
{
    return ui->lineEdit1->text();
}

QString QTwoEditDlg::getDlgEdit2()
{
    return ui->lineEdit2->text();
}

void QTwoEditDlg::setHelpText(const QString &helpText)
{
    ui->labelHelp->setText(helpText);
}
