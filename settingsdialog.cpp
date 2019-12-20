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

ZSettingsDialog::ZSettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ZSettingsDialog)
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
    ui->editRar->setPlaceholderText(QStringLiteral("rar.exe"));
#else
    ui->editRar->setPlaceholderText(QSL("rar"));
#endif

    connect(ui->btnDeleteBookmark,&QPushButton::clicked,this,&ZSettingsDialog::delListWidgetItem);
    connect(ui->btnBkColor,&QPushButton::clicked,this,&ZSettingsDialog::bkColorDlg);
    connect(ui->btnFontIndexer,&QPushButton::clicked,this,&ZSettingsDialog::idxFontDlg);
    connect(ui->btnFontOCR,&QPushButton::clicked,this,&ZSettingsDialog::ocrFontDlg);
    connect(ui->btnFrameColor,&QPushButton::clicked,this,&ZSettingsDialog::frameColorDlg);
    connect(ui->btnRar,&QToolButton::clicked,this,&ZSettingsDialog::openRar);

    connect(ui->btnDynAdd,&QPushButton::clicked,this,&ZSettingsDialog::dynAdd);
    connect(ui->btnDynEdit,&QPushButton::clicked,this,&ZSettingsDialog::dynEdit);
    connect(ui->btnDynDelete,&QPushButton::clicked,this,&ZSettingsDialog::delListWidgetItem);
    connect(ui->btnDeleteIgnored,&QPushButton::clicked,this,&ZSettingsDialog::delListWidgetItem);

    connect(ui->btnAddSearch, &QPushButton::clicked, this, &ZSettingsDialog::addSearchEngine);
    connect(ui->btnDelSearch, &QPushButton::clicked, this, &ZSettingsDialog::delSearchEngine);
    connect(ui->btnDefaultSearch, &QPushButton::clicked, this, &ZSettingsDialog::setDefaultSearch);

    connect(ui->rbMySQL, &QRadioButton::toggled, this, &ZSettingsDialog::updateSQLFields);
    connect(ui->rbSQLite, &QRadioButton::toggled, this, &ZSettingsDialog::updateSQLFields);

    connect(ui->btnOCRDatapath, &QPushButton::clicked, this, &ZSettingsDialog::ocrDatapathDlg);

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

ZSettingsDialog::~ZSettingsDialog()
{
    delete ui;
}

QColor ZSettingsDialog::getBkColor()
{
    return bkColor;
}

QFont ZSettingsDialog::getIdxFont()
{
    return idxFont;
}

QFont ZSettingsDialog::getOCRFont()
{
    return ocrFont;
}

QColor ZSettingsDialog::getFrameColor()
{
    return frameColor;
}

QStringList ZSettingsDialog::getIgnoredFiles()
{
    QStringList res;
    res.reserve(ui->listIgnoredFiles->count());
    for(int i=0;i<ui->listIgnoredFiles->count();i++)
        res << ui->listIgnoredFiles->item(i)->data(Qt::UserRole).toString();
    return res;
}

void ZSettingsDialog::setIgnoredFiles(const QStringList& files)
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

void ZSettingsDialog::setSearchEngines(const ZStrMap &engines)
{
    ui->listSearch->clear();
    for (auto it = engines.constKeyValueBegin(), end = engines.constKeyValueEnd(); it != end; ++it) {
        QListWidgetItem* li = new QListWidgetItem(QSL("%1 [ %2 ] %3").
                                                  arg((*it).first,(*it).second,
                (*it).first==zg->defaultSearchEngine ? tr("(default)") : QString()));
        li->setData(Qt::UserRole,(*it).first);
        li->setData(Qt::UserRole+1,(*it).second);
        ui->listSearch->addItem(li);
    }
}

ZStrMap ZSettingsDialog::getSearchEngines() const
{
    ZStrMap engines;
    engines.clear();
    for (int i=0; i<ui->listSearch->count(); i++)
        engines[ui->listSearch->item(i)->data(Qt::UserRole).toString()] =
                ui->listSearch->item(i)->data(Qt::UserRole+1).toString();
    return engines;

}

QString ZSettingsDialog::getOCRLanguage()
{
    return ui->comboOCRLanguage->currentData().toString();
}

QString ZSettingsDialog::getTranSourceLanguage()
{
    return ui->comboLangSource->currentData().toString();
}

QString ZSettingsDialog::getTranDestLanguage()
{
    return ui->comboLangDest->currentData().toString();
}

void ZSettingsDialog::updateOCRLanguages()
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

void ZSettingsDialog::updateTranslatorLanguages()
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

void ZSettingsDialog::addSearchEngine()
{
    ZStrMap data;
    data[tr("Url template")]=QString();
    data[tr("Menu title")]=QString();

    QString hlp = tr("In the url template you can use following substitutions\n"
                     "  %s - search text\n"
                     "  %ps - percent-encoded search text");

    ZMultiInputDialog *dlg = new ZMultiInputDialog(this,tr("Add new search engine"),data,hlp);
    if (dlg->exec()) {
        data = dlg->getInputData();

        if (getSearchEngines().keys().contains(data[tr("Menu title")]))
            QMessageBox::warning(this,tr("QManga"),tr("Unable to add two or more engines with same names.\n"
                                                      "Use another name for new engine."));
        else {
            QListWidgetItem* li = new QListWidgetItem(QSL("%1 [ %2 ] %3").
                                                      arg(data[tr("Menu title")],
                                                      data[tr("Url template")],
                    data[tr("Menu title")]==zg->defaultSearchEngine ? tr("(default)") : QString()));
            li->setData(Qt::UserRole,data[tr("Menu title")]);
            li->setData(Qt::UserRole+1,data[tr("Url template")]);
            ui->listSearch->addItem(li);
        }
    }
    dlg->deleteLater();
}

void ZSettingsDialog::delSearchEngine()
{
    QList<QListWidgetItem *> dl = ui->listSearch->selectedItems();
    for (QListWidgetItem* i : dl) {
        ui->listSearch->removeItemWidget(i);
        delete i;
    }
}

void ZSettingsDialog::setDefaultSearch()
{
    QList<QListWidgetItem *> dl = ui->listSearch->selectedItems();
    if (dl.isEmpty()) return;

    zg->defaultSearchEngine = dl.first()->data(Qt::UserRole).toString();

    setSearchEngines(getSearchEngines());
}

void ZSettingsDialog::updateSQLFields(bool checked)
{
    Q_UNUSED(checked)

    bool useLogin = ui->rbMySQL->isChecked();
    ui->editMySqlBase->setEnabled(useLogin);
    ui->editMySqlHost->setEnabled(useLogin);
    ui->editMySqlLogin->setEnabled(useLogin);
    ui->editMySqlPassword->setEnabled(useLogin);
}

void ZSettingsDialog::ocrDatapathDlg()
{
#ifdef WITH_OCR
    QString datapath = getExistingDirectoryD(this,tr("Tesseract datapath"),ui->editOCRDatapath->text());
    if (!datapath.isEmpty())
        ui->editOCRDatapath->setText(datapath);
    updateOCRLanguages();
#endif
}

void ZSettingsDialog::delListWidgetItem()
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

void ZSettingsDialog::bkColorDlg()
{
    QColor c = QColorDialog::getColor(bkColor,this);
    if (!c.isValid()) return;
    updateBkColor(c);
}

void ZSettingsDialog::idxFontDlg()
{
    bool ok;
    QFont f = QFontDialog::getFont(&ok,idxFont,this);
    if (!ok) return;
    updateIdxFont(f);
}

void ZSettingsDialog::ocrFontDlg()
{
    bool ok;
    QFont f = QFontDialog::getFont(&ok,ocrFont,this);
    if (!ok) return;
    updateOCRFont(f);
}

void ZSettingsDialog::frameColorDlg()
{
    QColor c = QColorDialog::getColor(frameColor,this);
    if (!c.isValid()) return;
    updateFrameColor(c);
}

void ZSettingsDialog::updateBkColor(const QColor &c)
{
    bkColor = c;
    QPalette p = ui->frameBkColor->palette();
    p.setBrush(QPalette::Window,QBrush(bkColor));
    ui->frameBkColor->setPalette(p);
}

void ZSettingsDialog::updateIdxFont(const QFont &f)
{
    idxFont = f;
    ui->labelIdxFont->setFont(idxFont);
    ui->labelIdxFont->setText(QSL("%1, %2").arg(f.family()).arg(f.pointSize()));
}

void ZSettingsDialog::updateOCRFont(const QFont &f)
{
    ocrFont = f;
    ui->labelOCRFont->setFont(ocrFont);
    ui->labelOCRFont->setText(QSL("%1, %2").arg(f.family()).arg(f.pointSize()));
}

void ZSettingsDialog::updateFrameColor(const QColor &c)
{
    frameColor = c;
    QPalette p = ui->frameFrameColor->palette();
    p.setBrush(QPalette::Window,QBrush(frameColor));
    ui->frameFrameColor->setPalette(p);
}

void ZSettingsDialog::dynAdd()
{
    ZTwoEditDlg *dlg = new ZTwoEditDlg(this,tr("Add dynamic list"),tr("List title"),
                                       tr("Query part"));
    dlg->setHelpText(tr("Query part contains part of SELECT query on manga table "
                        "from FROM clause to the end of query.\n"
                        "This part can consists of WHERE, ORDER, LIMIT and any other MySQL SELECT clauses."));

    connect(zg->db,&ZDB::gotTablesDescription,dlg,&ZTwoEditDlg::setAuxText,Qt::QueuedConnection);
    connect(this,&ZSettingsDialog::getTablesDescription,
            zg->db,&ZDB::sqlGetTablesDescription,Qt::QueuedConnection);
    Q_EMIT getTablesDescription();

    if (dlg->exec()) {
        QListWidgetItem* li = new QListWidgetItem(QSL("%1 [ %2 ]").
                                                  arg(dlg->getDlgEdit1(),
                                                  dlg->getDlgEdit2()));
        li->setData(Qt::UserRole,dlg->getDlgEdit1());
        li->setData(Qt::UserRole+1,dlg->getDlgEdit2());
        ui->listDynAlbums->addItem(li);
    }
    dlg->setParent(nullptr);
    delete dlg;
}

void ZSettingsDialog::dynEdit()
{
    if (ui->listDynAlbums->selectedItems().isEmpty()) return;
    ZTwoEditDlg *dlg = new ZTwoEditDlg(this,tr("Edit dynamic list"),tr("List title"),
                                       tr("Query part"),
                                       ui->listDynAlbums->selectedItems().first()->data(Qt::UserRole).toString(),
                                       ui->listDynAlbums->selectedItems().first()->data(Qt::UserRole+1).toString());
    dlg->setHelpText(tr("Query part contains part of SELECT query on manga table "
                        "from FROM clause to the end of query.\n"
                        "This part can consists of WHERE, ORDER, LIMIT and any other MySQL SELECT clauses."));
    if (dlg->exec()) {
        ui->listDynAlbums->selectedItems().first()->setText(QSL("%1 [ %2 ]").
                                                            arg(dlg->getDlgEdit1(),
                                                            dlg->getDlgEdit2()));
        ui->listDynAlbums->selectedItems().first()->setData(Qt::UserRole,dlg->getDlgEdit1());
        ui->listDynAlbums->selectedItems().first()->setData(Qt::UserRole+1,dlg->getDlgEdit2());
    }
    dlg->setParent(nullptr);
    delete dlg;
}

void ZSettingsDialog::openRar()
{
    QString filter = QSL("*");
#ifdef Q_OS_WIN
    filter = tr("Executable files (*.exe)");
#endif
    QString filename = getOpenFileNameD(this,tr("Select console RAR binary"),zg->savedAuxOpenDir,filter);
    if (!filename.isEmpty())
        ui->editRar->setText(filename);
}
