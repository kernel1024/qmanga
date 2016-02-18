#include <QColorDialog>
#include <QFontDialog>

#include "settingsdialog.h"
#include "bookmarkdlg.h"
#include "ui_settingsdialog.h"
#include "zglobal.h"
#include "zdb.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    bkColor = QApplication::palette("QWidget").dark().color();
    idxFont = QApplication::font("QListView");
    ocrFont = QApplication::font("QPlainTextView");
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
    listDynAlbums = ui->listDynAlbums;
    comboPDFRendererMode = ui->comboPDFRenderMode;
    checkForceDPI = ui->checkPDFDPI;
    spinForceDPI = ui->spinPDFDPI;

#ifndef WITH_MAGICK
    while (ui->comboFilter->count()>2)
        ui->comboFilter->removeItem(ui->comboFilter->count()-1);
#endif

    connect(ui->btnDeleteBookmark,SIGNAL(clicked()),this,SLOT(delBookmark()));
    connect(ui->btnBkColor,SIGNAL(clicked()),this,SLOT(bkColorDlg()));
    connect(ui->btnFontIndexer,SIGNAL(clicked()),this,SLOT(idxFontDlg()));
    connect(ui->btnFontOCR,SIGNAL(clicked()),this,SLOT(ocrFontDlg()));
    connect(ui->btnFrameColor,SIGNAL(clicked()),this,SLOT(frameColorDlg()));

    connect(ui->btnDynAdd,SIGNAL(clicked()),this,SLOT(dynAdd()));
    connect(ui->btnDynEdit,SIGNAL(clicked()),this,SLOT(dynEdit()));
    connect(ui->btnDynDelete,SIGNAL(clicked()),this,SLOT(dynDelete()));
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

QFont SettingsDialog::getOCRFont()
{
    return ocrFont;
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

void SettingsDialog::ocrFontDlg()
{
    bool ok;
    QFont f = QFontDialog::getFont(&ok,ocrFont,this);
    if (!ok) return;
    updateOCRFont(f);
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

void SettingsDialog::updateOCRFont(QFont f)
{
    ocrFont = f;
    ui->labelOCRFont->setFont(ocrFont);
    ui->labelOCRFont->setText(QString("%1, %2").arg(f.family()).arg(f.pointSize()));
}

void SettingsDialog::updateFrameColor(QColor c)
{
    frameColor = c;
    QPalette p = ui->frameFrameColor->palette();
    p.setBrush(QPalette::Window,QBrush(frameColor));
    ui->frameFrameColor->setPalette(p);
}

void SettingsDialog::dynAdd()
{
    QTwoEditDlg *dlg = new QTwoEditDlg(this,tr("Add dynamic list"),tr("List title"),
                                       tr("Query part"));
    dlg->setHelpText(tr("Query part contains part of SELECT query on manga table "
                        "from FROM clause to the end of query.\n"
                        "This part can consists of WHERE, ORDER, LIMIT and any other MySQL SELECT clauses."));

    connect(zg->db,SIGNAL(gotTablesDescription(QString)),dlg,SLOT(setAuxText(QString)),Qt::QueuedConnection);
    QMetaObject::invokeMethod(zg->db,"sqlGetTablesDescription",Qt::QueuedConnection);

    if (dlg->exec()) {
        QListWidgetItem* li = new QListWidgetItem(QString("%1 [ %2 ]").
                                                  arg(dlg->getDlgEdit1()).
                                                  arg(dlg->getDlgEdit2()));
        li->setData(Qt::UserRole,dlg->getDlgEdit1());
        li->setData(Qt::UserRole+1,dlg->getDlgEdit2());
        ui->listDynAlbums->addItem(li);
    }
    dlg->setParent(NULL);
    delete dlg;
}

void SettingsDialog::dynEdit()
{
    if (ui->listDynAlbums->selectedItems().isEmpty()) return;
    QTwoEditDlg *dlg = new QTwoEditDlg(this,tr("Edit dynamic list"),tr("List title"),
                                       tr("Query part"),
                                       ui->listDynAlbums->selectedItems().first()->data(Qt::UserRole).toString(),
                                       ui->listDynAlbums->selectedItems().first()->data(Qt::UserRole+1).toString());
    dlg->setHelpText(tr("Query part contains part of SELECT query on manga table "
                        "from FROM clause to the end of query.\n"
                        "This part can consists of WHERE, ORDER, LIMIT and any other MySQL SELECT clauses."));
    if (dlg->exec()) {
        ui->listDynAlbums->selectedItems().first()->setText(QString("%1 [ %2 ]").
                                                            arg(dlg->getDlgEdit1()).
                                                            arg(dlg->getDlgEdit2()));
        ui->listDynAlbums->selectedItems().first()->setData(Qt::UserRole,dlg->getDlgEdit1());
        ui->listDynAlbums->selectedItems().first()->setData(Qt::UserRole+1,dlg->getDlgEdit2());
    }
    dlg->setParent(NULL);
    delete dlg;
}

void SettingsDialog::dynDelete()
{
    QList<QListWidgetItem *> dl = ui->listDynAlbums->selectedItems();
    foreach (QListWidgetItem* i, dl) {
        ui->listDynAlbums->removeItemWidget(i);
        delete i;
    }
}
