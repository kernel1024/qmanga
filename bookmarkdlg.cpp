#include "bookmarkdlg.h"
#include "ui_bookmarkdlg.h"

QBookmarkDlg::QBookmarkDlg(QWidget *parent, const QString &name, const QString &filename) :
    QDialog(parent),
    ui(new Ui::QBookmarkDlg)
{
    ui->setupUi(this);
    ui->lineTitle->setText(name);
    ui->lineFile->setText(filename);
}

QBookmarkDlg::~QBookmarkDlg()
{
    delete ui;
}

QString QBookmarkDlg::getBkTitle()
{
    return ui->lineTitle->text();
}

QString QBookmarkDlg::getBkFilename()
{
    return ui->lineFile->text();
}
