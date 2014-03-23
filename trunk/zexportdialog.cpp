#include "zexportdialog.h"
#include "ui_zexportdialog.h"
#include <QFileDialog>

ZExportDialog::ZExportDialog(QWidget *parent, const int pagesMaximum) :
    QDialog(parent),
    ui(new Ui::ZExportDialog)
{
    ui->setupUi(this);

    ui->exportDir->setText(QDir::currentPath());
    ui->pageCount->setMaximum(pagesMaximum);

    connect(ui->exportDirBtn,SIGNAL(clicked()),this,SLOT(dirSelectBtn()));
}

ZExportDialog::~ZExportDialog()
{
    delete ui;
}

QString ZExportDialog::getExportDir()
{
    return ui->exportDir->text();
}

int ZExportDialog::getPagesCount()
{
    return ui->pageCount->value();
}

int ZExportDialog::getImageQuality()
{
    return ui->imageQuality->value();
}

QString ZExportDialog::getImageFormat()
{
    return ui->imageFormat->currentText();
}

void ZExportDialog::dirSelectBtn()
{
    QString s = QFileDialog::getExistingDirectory(this,tr("Export to directory"),ui->exportDir->text());
    QFileInfo fi(s);
    if (fi.exists())
        ui->exportDir->setText(s);
}
