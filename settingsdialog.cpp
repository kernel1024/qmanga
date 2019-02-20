#include <QColorDialog>
#include <QFontDialog>
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QMessageBox>

#include "settingsdialog.h"
#include "bookmarkdlg.h"
#include "multiinputdialog.h"
#include "ui_settingsdialog.h"
#include "zglobal.h"
#include "global.h"
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

    comboUpscaleFilter = ui->comboUpscaleFilter;
    comboDownscaleFilter = ui->comboDownscaleFilter;
    spinCacheWidth = ui->spinCacheWidth;
    spinMagnify = ui->spinMagnify;
    editMySqlHost = ui->editMySqlHost;
    editMySqlBase = ui->editMySqlBase;
    editMySqlLogin = ui->editMySqlLogin;
    editMySqlPassword = ui->editMySqlPassword;
    editRar = ui->editRar;
    listBookmarks = ui->listBookmarks;
    radioCachePixmaps = ui->rbCachePixmaps;
    radioCacheData = ui->rbCacheData;
    checkFineRendering = ui->checkFineRendering;
    checkFSWatcher = ui->checkFSWatcher;
    spinScrollDelta = ui->spinScrollDelta;
    spinScrollFactor = ui->spinScrollFactor;
    spinSearchScrollFactor = ui->spinSearchScrollFactor;
    labelDetectedDelta = ui->labelDetectedDelta;
    listDynAlbums = ui->listDynAlbums;
    comboPDFRendererMode = ui->comboPDFRenderMode;
    checkForceDPI = ui->checkPDFDPI;
    spinForceDPI = ui->spinPDFDPI;
    spinBlur = ui->spinBlur;
    radioMySQL = ui->rbMySQL;
    radioSQLite = ui->rbSQLite;
    editOCRDatapath = ui->editOCRDatapath;

#ifdef Q_OS_WIN
    ui->editRar->setPlaceholderText("rar.exe");
#else
    ui->editRar->setPlaceholderText("rar");
#endif

    connect(ui->btnDeleteBookmark,&QPushButton::clicked,this,&SettingsDialog::delListWidgetItem);
    connect(ui->btnBkColor,&QPushButton::clicked,this,&SettingsDialog::bkColorDlg);
    connect(ui->btnFontIndexer,&QPushButton::clicked,this,&SettingsDialog::idxFontDlg);
    connect(ui->btnFontOCR,&QPushButton::clicked,this,&SettingsDialog::ocrFontDlg);
    connect(ui->btnFrameColor,&QPushButton::clicked,this,&SettingsDialog::frameColorDlg);
    connect(ui->btnRar,&QToolButton::clicked,this,&SettingsDialog::openRar);

    connect(ui->btnDynAdd,&QPushButton::clicked,this,&SettingsDialog::dynAdd);
    connect(ui->btnDynEdit,&QPushButton::clicked,this,&SettingsDialog::dynEdit);
    connect(ui->btnDynDelete,&QPushButton::clicked,this,&SettingsDialog::delListWidgetItem);
    connect(ui->btnDeleteIgnored,&QPushButton::clicked,this,&SettingsDialog::delListWidgetItem);

    connect(ui->btnAddSearch, &QPushButton::clicked, this, &SettingsDialog::addSearchEngine);
    connect(ui->btnDelSearch, &QPushButton::clicked, this, &SettingsDialog::delSearchEngine);
    connect(ui->btnDefaultSearch, &QPushButton::clicked, this, &SettingsDialog::setDefaultSearch);

    connect(ui->rbMySQL, &QRadioButton::toggled, this, &SettingsDialog::updateSQLFields);
    connect(ui->rbSQLite, &QRadioButton::toggled, this, &SettingsDialog::updateSQLFields);

    connect(ui->btnOCRDatapath, &QPushButton::clicked, this, &SettingsDialog::ocrDatapathDlg);

    delLookup[ui->btnDeleteBookmark] = ui->listBookmarks;
    delLookup[ui->btnDynDelete] = ui->listDynAlbums;
    delLookup[ui->btnDeleteIgnored] = ui->listIgnoredFiles;

#ifndef WITH_OCR
    ui->groupTesseract->setEnabled(false);
#else
    ui->editOCRDatapath->setText(ocrGetDatapath());
    updateOCRLanguages();
#endif

#ifdef Q_OS_WIN
    ui->groupTranslator->setEnabled(false);
#else
    updateTranslatorLanguages();
#endif
    updateSQLFields(false);
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
    for(const QString& fname : files) {
        QFileInfo fi(fname);
        QListWidgetItem* li = new QListWidgetItem(fi.fileName());
        li->setData(Qt::UserRole,fname);
        li->setToolTip(fname);
        ui->listIgnoredFiles->addItem(li);
    }
}

void SettingsDialog::setSearchEngines(const ZStrMap &engines)
{
    ui->listSearch->clear();
    for (auto it = engines.keyValueBegin(), end = engines.keyValueEnd(); it != end; ++it) {
        QListWidgetItem* li = new QListWidgetItem(QString("%1 [ %2 ] %3").
                                                  arg((*it).first,(*it).second,
                (*it).first==zg->defaultSearchEngine ? tr("(default)") : QString()));
        li->setData(Qt::UserRole,(*it).first);
        li->setData(Qt::UserRole+1,(*it).second);
        ui->listSearch->addItem(li);
    }
}

ZStrMap SettingsDialog::getSearchEngines() const
{
    ZStrMap engines;
    engines.clear();
    for (int i=0; i<ui->listSearch->count(); i++)
        engines[ui->listSearch->item(i)->data(Qt::UserRole).toString()] =
                ui->listSearch->item(i)->data(Qt::UserRole+1).toString();
    return engines;

}

QString SettingsDialog::getOCRLanguage()
{
    return ui->comboOCRLanguage->currentData().toString();
}

QString SettingsDialog::getTranSourceLanguage()
{
    return ui->comboLangSource->currentData().toString();
}

QString SettingsDialog::getTranDestLanguage()
{
    return ui->comboLangDest->currentData().toString();
}

void SettingsDialog::updateOCRLanguages()
{
#ifdef WITH_OCR
    ui->comboOCRLanguage->clear();
    QString selectedLang = ocrGetActiveLanguage();

    QDir datapath(ui->editOCRDatapath->text());
    if (datapath.isReadable()) {
        int aidx = -1;
        QFileInfoList files = datapath.entryInfoList({"*.traineddata"});
        for (const QFileInfo& fi : files) {
            QString base = fi.baseName();
            ui->comboOCRLanguage->addItem(fi.fileName(),base);
            if (base==selectedLang)
                aidx = ui->comboOCRLanguage->count()-1;
        }

        if (aidx>=0)
            ui->comboOCRLanguage->setCurrentIndex(aidx);
        else if (ui->comboOCRLanguage->count()>0)
            ui->comboOCRLanguage->setCurrentIndex(0);
    }
#endif
}

void SettingsDialog::updateTranslatorLanguages()
{
#ifndef Q_OS_WIN
    ui->comboLangSource->clear();
    ui->comboLangDest->clear();
    const QStringList sl = zg->getLanguageCodes();
    int idx1 = -1;
    int idx2 = -1;
    for (const QString& bcp : sl) {
        ui->comboLangSource->addItem(zg->getLanguageName(bcp),QVariant::fromValue(bcp));
        ui->comboLangDest->addItem(zg->getLanguageName(bcp),QVariant::fromValue(bcp));
        if (bcp==zg->tranSourceLang) idx1 = ui->comboLangSource->count()-1;
        if (bcp==zg->tranDestLang) idx2 = ui->comboLangDest->count()-1;
    }
    if (idx1>=0) ui->comboLangSource->setCurrentIndex(idx1);
    else if (ui->comboLangSource->count()>0) ui->comboLangSource->setCurrentIndex(0);
    if (idx2>=0) ui->comboLangDest->setCurrentIndex(idx2);
    else if (ui->comboLangDest->count()>0) ui->comboLangDest->setCurrentIndex(0);
#endif
}

void SettingsDialog::addSearchEngine()
{
    ZStrMap data;
    data["Url template"]=QString();
    data["Menu title"]=QString();

    QString hlp = tr("In the url template you can use following substitutions\n"
                     "  %s - search text\n"
                     "  %ps - percent-encoded search text");

    CMultiInputDialog *dlg = new CMultiInputDialog(this,tr("Add new search engine"),data,hlp);
    if (dlg->exec()) {
        data = dlg->getInputData();

        if (getSearchEngines().keys().contains(data["Menu title"]))
            QMessageBox::warning(this,tr("QManga"),tr("Unable to add two or more engines with same names.\n"
                                                      "Use another name for new engine."));
        else {
            QListWidgetItem* li = new QListWidgetItem(QString("%1 [ %2 ] %3").
                                                      arg(data["Menu title"],
                                                      data["Url template"],
                    data["Menu title"]==zg->defaultSearchEngine ? tr("(default)") : QString()));
            li->setData(Qt::UserRole,data["Menu title"]);
            li->setData(Qt::UserRole+1,data["Url template"]);
            ui->listSearch->addItem(li);
        }
    }
    dlg->deleteLater();
}

void SettingsDialog::delSearchEngine()
{
    QList<QListWidgetItem *> dl = ui->listSearch->selectedItems();
    for (QListWidgetItem* i : dl) {
        ui->listBookmarks->removeItemWidget(i);
        delete i;
    }
}

void SettingsDialog::setDefaultSearch()
{
    QList<QListWidgetItem *> dl = ui->listSearch->selectedItems();
    if (dl.isEmpty()) return;

    zg->defaultSearchEngine = dl.first()->data(Qt::UserRole).toString();

    setSearchEngines(getSearchEngines());
}

void SettingsDialog::updateSQLFields(bool checked)
{
    Q_UNUSED(checked)

    bool useLogin = ui->rbMySQL->isChecked();
    ui->editMySqlBase->setEnabled(useLogin);
    ui->editMySqlHost->setEnabled(useLogin);
    ui->editMySqlLogin->setEnabled(useLogin);
    ui->editMySqlPassword->setEnabled(useLogin);
}

void SettingsDialog::ocrDatapathDlg()
{
#ifdef WITH_OCR
    QString datapath = getExistingDirectoryD(this,tr("Tesseract datapath"),ui->editOCRDatapath->text());
    if (!datapath.isEmpty())
        ui->editOCRDatapath->setText(datapath);
    updateOCRLanguages();
#endif
}

void SettingsDialog::delListWidgetItem()
{
    auto btn = qobject_cast<QPushButton *>(sender());
    if (btn==nullptr || !delLookup.contains(btn)) return;
    QListWidget* list = delLookup.value(btn);
    if (list==nullptr) return;

    QList<QListWidgetItem *> dl = list->selectedItems();
    for (QListWidgetItem* i : dl) {
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

void SettingsDialog::updateBkColor(const QColor &c)
{
    bkColor = c;
    QPalette p = ui->frameBkColor->palette();
    p.setBrush(QPalette::Window,QBrush(bkColor));
    ui->frameBkColor->setPalette(p);
}

void SettingsDialog::updateIdxFont(const QFont &f)
{
    idxFont = f;
    ui->labelIdxFont->setFont(idxFont);
    ui->labelIdxFont->setText(QString("%1, %2").arg(f.family()).arg(f.pointSize()));
}

void SettingsDialog::updateOCRFont(const QFont &f)
{
    ocrFont = f;
    ui->labelOCRFont->setFont(ocrFont);
    ui->labelOCRFont->setText(QString("%1, %2").arg(f.family()).arg(f.pointSize()));
}

void SettingsDialog::updateFrameColor(const QColor &c)
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

    connect(zg->db,&ZDB::gotTablesDescription,dlg,&QTwoEditDlg::setAuxText,Qt::QueuedConnection);
    QMetaObject::invokeMethod(zg->db,"sqlGetTablesDescription",Qt::QueuedConnection);

    if (dlg->exec()) {
        QListWidgetItem* li = new QListWidgetItem(QString("%1 [ %2 ]").
                                                  arg(dlg->getDlgEdit1(),
                                                  dlg->getDlgEdit2()));
        li->setData(Qt::UserRole,dlg->getDlgEdit1());
        li->setData(Qt::UserRole+1,dlg->getDlgEdit2());
        ui->listDynAlbums->addItem(li);
    }
    dlg->setParent(nullptr);
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
                                                            arg(dlg->getDlgEdit1(),
                                                            dlg->getDlgEdit2()));
        ui->listDynAlbums->selectedItems().first()->setData(Qt::UserRole,dlg->getDlgEdit1());
        ui->listDynAlbums->selectedItems().first()->setData(Qt::UserRole+1,dlg->getDlgEdit2());
    }
    dlg->setParent(nullptr);
    delete dlg;
}

void SettingsDialog::openRar()
{
    QString filter = QString("*");
#ifdef Q_OS_WIN
    filter = tr("Executable files (*.exe)");
#endif
    QString filename = getOpenFileNameD(this,tr("Select console RAR binary"),zg->savedAuxOpenDir,filter);
    if (!filename.isEmpty())
        ui->editRar->setText(filename);
}
