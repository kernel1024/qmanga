#include "zexportdialog.h"
#include "ui_zexportdialog.h"
#include <QFileDialog>

ZExportDialog::ZExportDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ZExportDialog)
{
    ui->setupUi(this);

    ui->exportDir->setText(QDir::currentPath());

    connect(ui->exportDirBtn,SIGNAL(clicked()),this,SLOT(dirSelectBtn()));
}

ZExportDialog::~ZExportDialog()
{
    delete ui;
}

void ZExportDialog::setPagesMaximum(const int pagesMaximum)
{
    ui->pageCount->setMaximum(pagesMaximum);
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
