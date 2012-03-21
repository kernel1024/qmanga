#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    comboFilter = ui->comboFilter;
    spinCacheWidth = ui->spinCacheWidth;
    spinMagnify = ui->spinMagnify;
    editMySqlLogin = ui->editMySqlLogin;
    editMySqlPassword = ui->editMySqlPassword;
    listBookmarks = ui->listBookmarks;


    connect(ui->btnDeleteBookmark,SIGNAL(clicked()),this,SLOT(delBookmark()));
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::delBookmark()
{
    QList<QListWidgetItem *> dl = ui->listBookmarks->selectedItems();
    foreach (QListWidgetItem* i, dl) {
        ui->listBookmarks->removeItemWidget(i);
        delete i;
    }
}
