#include "bookmarkdlg.h"
#include "ui_bookmarkdlg.h"

ZTwoEditDlg::ZTwoEditDlg(QWidget *parent, const QString& windowTitle,
                         const QString &title1, const QString &title2,
                         const QString &value1, const QString &value2) :
    QDialog(parent),
    ui(new Ui::ZTwoEditDlg)
{
    ui->setupUi(this);
    setWindowTitle(windowTitle);
    ui->label1->setText(title1);
    ui->label2->setText(title2);
    ui->lineEdit1->setText(value1);
    ui->lineEdit2->setText(value2);
    ui->labelHelp->clear();
    ui->labelAux->clear();
}

ZTwoEditDlg::~ZTwoEditDlg()
{
    delete ui;
}

QString ZTwoEditDlg::getDlgEdit1()
{
    return ui->lineEdit1->text();
}

QString ZTwoEditDlg::getDlgEdit2()
{
    return ui->lineEdit2->text();
}

void ZTwoEditDlg::setHelpText(const QString &helpText)
{
    ui->labelHelp->setText(helpText);
}

void ZTwoEditDlg::setAuxText(const QString &text)
{
    ui->labelAux->setText(text);
}
