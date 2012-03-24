#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    bkColor = QApplication::palette("QWidget").dark().color();
    idxFont = QApplication::font("QListView");

    comboFilter = ui->comboFilter;
    spinCacheWidth = ui->spinCacheWidth;
    spinMagnify = ui->spinMagnify;
    editMySqlBase = ui->editMySqlBase;
    editMySqlLogin = ui->editMySqlLogin;
    editMySqlPassword = ui->editMySqlPassword;
    listBookmarks = ui->listBookmarks;

    connect(ui->btnDeleteBookmark,SIGNAL(clicked()),this,SLOT(delBookmark()));
    connect(ui->btnBkColor,SIGNAL(clicked()),this,SLOT(bkColorDlg()));
    connect(ui->btnFontIndexer,SIGNAL(clicked()),this,SLOT(idxFontDlg()));
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

QColor SettingsDialog::getBkColor()
{
    return bkColor;
}

QFont SettingsDialog::getIdxFont()
{
    return idxFont;
}

void SettingsDialog::delBookmark()
{
    QList<QListWidgetItem *> dl = ui->listBookmarks->selectedItems();
    foreach (QListWidgetItem* i, dl) {
        ui->listBookmarks->removeItemWidget(i);
        delete i;
    }
}

void SettingsDialog::bkColorDlg()
{
    QColor c = QColorDialog::getColor(bkColor,this);
    if (!c.isValid()) return;
    updateBkColor(c);
}

void SettingsDialog::idxFontDlg()
{
    bool ok;
    QFont f = QFontDialog::getFont(&ok,idxFont,this);
    if (!ok) return;
    updateIdxFont(f);
}

void SettingsDialog::updateBkColor(QColor c)
{
    bkColor = c;
    QPalette p = ui->frameBkColor->palette();
    p.setBrush(QPalette::Window,QBrush(bkColor));
    ui->frameBkColor->setPalette(p);
}

void SettingsDialog::updateIdxFont(QFont f)
{
    idxFont = f;
    ui->labelIdxFont->setFont(idxFont);
    ui->labelIdxFont->setText(QString("%1, %2").arg(f.family()).arg(f.pointSize()));
}
