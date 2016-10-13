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

    connect(ui->btnDeleteBookmark,&QPushButton::clicked,this,&SettingsDialog::delListWidgetItem);
    connect(ui->btnBkColor,&QPushButton::clicked,this,&SettingsDialog::bkColorDlg);
    connect(ui->btnFontIndexer,&QPushButton::clicked,this,&SettingsDialog::idxFontDlg);
    connect(ui->btnFontOCR,&QPushButton::clicked,this,&SettingsDialog::ocrFontDlg);
    connect(ui->btnFrameColor,&QPushButton::clicked,this,&SettingsDialog::frameColorDlg);

    connect(ui->btnDynAdd,&QPushButton::clicked,this,&SettingsDialog::dynAdd);
    connect(ui->btnDynEdit,&QPushButton::clicked,this,&SettingsDialog::dynEdit);
    connect(ui->btnDynDelete,&QPushButton::clicked,this,&SettingsDialog::delListWidgetItem);
    connect(ui->btnDeleteIgnored,&QPushButton::clicked,this,&SettingsDialog::delListWidgetItem);

    delLookup[ui->btnDeleteBookmark] = ui->listBookmarks;
    delLookup[ui->btnDynDelete] = ui->listDynAlbums;
    delLookup[ui->btnDeleteIgnored] = ui->listIgnoredFiles;
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

QStringList SettingsDialog::getIgnoredFiles()
{
    QStringList res;
    for(int i=0;i<ui->listIgnoredFiles->count();i++)
        res << ui->listIgnoredFiles->item(i)->data(Qt::UserRole).toString();
    return res;
}

void SettingsDialog::setIgnoredFiles(const QStringList& files)
{
    ui->listIgnoredFiles->clear();
    foreach(const QString& fname, files) {
        QFileInfo fi(fname);
        QListWidgetItem* li = new QListWidgetItem(fi.fileName());
        li->setData(Qt::UserRole,fname);
        li->setToolTip(fname);
        ui->listIgnoredFiles->addItem(li);
    }
}

void SettingsDialog::delListWidgetItem()
{
    QPushButton* btn = qobject_cast<QPushButton *>(sender());
    if (btn==NULL || !delLookup.contains(btn)) return;
    QListWidget* list = delLookup.value(btn);
    if (list==NULL) return;

    QList<QListWidgetItem *> dl = list->selectedItems();
    foreach (QListWidgetItem* i, dl) {
        list->removeItemWidget(i);
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
