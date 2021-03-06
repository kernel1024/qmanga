#include "zexportdialog.h"
#include "ui_zexportdialog.h"
#include <QFileDialog>

ZExportDialog::ZExportDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ZExportDialog)
{
    m_currentPage = 0;

    ui->setupUi(this);

    ui->exportDir->setText(QDir::currentPath());

    connect(ui->exportDirBtn,&QPushButton::clicked,this,&ZExportDialog::dirSelectBtn);
    connect(ui->pageCount,qOverload<int>(&QSpinBox::valueChanged),this,&ZExportDialog::updateRange);
}

ZExportDialog::~ZExportDialog()
{
    delete ui;
}

void ZExportDialog::setPages(int currentPage, int pagesMaximum)
{
    m_currentPage = currentPage;
    ui->pageCount->setMaximum(pagesMaximum);
    updateRange(ui->pageCount->value());
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

void ZExportDialog::setExportDir(const QString &dir)
{
    ui->exportDir->setText(dir);
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

void ZExportDialog::updateRange(int value)
{
    ui->labelRange->setText(tr("From %1 to %2.").arg(m_currentPage+1).arg(m_currentPage+value));
}
