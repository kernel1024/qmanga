#include <QColorDialog>
#include <QFontDialog>

#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    bkColor = QApplication::palette("QWidget").dark().color();
    idxFont = QApplication::font("QListView");
    frameColor = QColor(Qt::lightGray);

    comboFilter = ui->comboFilter;
    spinCacheWidth = ui->spinCacheWidth;
    spinMagnify = ui->spinMagnify;
    editMySqlBase = ui->editMySqlBase;
    editMySqlLogin = ui->editMySqlLogin;
    editMySqlPassword = ui->editMySqlPassword;
    listBookmarks = ui->listBookmarks;
    radioCachePixmaps = ui->rbCachePixmaps;
    radioCacheData = ui->rbCacheData;
    checkFineRendering = ui->checkFineRendering;
    checkFSWatcher = ui->checkFSWatcher;
    spinScrollDelta = ui->spinScrollDelta;
    labelDetectedDelta = ui->labelDetectedDelta;

#ifndef WITH_MAGICK
    while (ui->comboFilter->count()>2)
        ui->comboFilter->removeItem(ui->comboFilter->count()-1);
#endif

    connect(ui->btnDeleteBookmark,SIGNAL(clicked()),this,SLOT(delBookmark()));
    connect(ui->btnBkColor,SIGNAL(clicked()),this,SLOT(bkColorDlg()));
    connect(ui->btnFontIndexer,SIGNAL(clicked()),this,SLOT(idxFontDlg()));
    connect(ui->btnFrameColor,SIGNAL(clicked()),this,SLOT(frameColorDlg()));
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

QColor SettingsDialog::getFrameColor()
{
    return frameColor;
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

void SettingsDialog::frameColorDlg()
{
    QColor c = QColorDialog::getColor(frameColor,this);
    if (!c.isValid()) return;
    updateFrameColor(c);
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

void SettingsDialog::updateFrameColor(QColor c)
{
    frameColor = c;
    QPalette p = ui->frameFrameColor->palette();
    p.setBrush(QPalette::Window,QBrush(frameColor));
    ui->frameFrameColor->setPalette(p);
}
