#include <QColorDialog>
#include <QFontDialog>
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QMessageBox>
#include <QPageSize>

#include "settingsdialog.h"
#include "bookmarkdlg.h"
#include "multiinputdialog.h"
#include "ui_settingsdialog.h"
#include "zglobal.h"
#include "global.h"
#include "zdb.h"
#include "ocr/tesseractocr.h"

ZSettingsDialog::ZSettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ZSettingsDialog)
{
    ui->setupUi(this);

#ifdef Q_OS_WIN
    ui->editRar->setPlaceholderText(QStringLiteral("rar.exe"));
#else
    ui->editRar->setPlaceholderText(QSL("rar"));
#endif

    connect(ui->btnDeleteBookmark,&QPushButton::clicked,this,&ZSettingsDialog::delListWidgetItem);
    connect(ui->btnFontIndexer,&QPushButton::clicked,this,&ZSettingsDialog::idxFontDlg);
    connect(ui->btnFontOCR,&QPushButton::clicked,this,&ZSettingsDialog::ocrFontDlg);
    connect(ui->btnFontText,&QPushButton::clicked,this,&ZSettingsDialog::textDocFontDlg);
    connect(ui->btnBkColor,&QPushButton::clicked,this,&ZSettingsDialog::bkColorDlg);
    connect(ui->btnFrameColor,&QPushButton::clicked,this,&ZSettingsDialog::frameColorDlg);
    connect(ui->btnTextBkColor,&QPushButton::clicked,this,&ZSettingsDialog::textDocBkColorDlg);
    connect(ui->btnRar,&QToolButton::clicked,this,&ZSettingsDialog::openRar);
    connect(ui->btnOffice,&QToolButton::clicked,this,&ZSettingsDialog::openOffice);

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
    connect(ui->btnGCPKeyFile, &QPushButton::clicked, this, &ZSettingsDialog::gcpKeyFileDlg);

    connect(ui->comboTextPageSize, &QComboBox::currentIndexChanged, this, &ZSettingsDialog::updateTextPageSizes);

#ifndef WITH_TESSERACT
    ui->rbTesseract->setEnabled(false);
    ui->groupTesseract->setEnabled(false);
#else
    ui->editOCRDatapath->setText(ZTesseractOCR::tesseractDatapath());
    updateOCRLanguages();
#endif

#ifdef Q_OS_WIN
    ui->groupTranslator->setEnabled(false);
#else
    updateTranslatorLanguages();
#endif
    updateSQLFields(false);

    updateBkColor(QApplication::palette("QWidget").dark().color());
    updateTextDocBkColor(QApplication::palette("QWidget").base().color());
    updateFrameColor(QColor(Qt::lightGray));
    updateIdxFont(QApplication::font("QListView"));
    updateOCRFont(QApplication::font("QPlainTextView"));
    updateTextDocFont(QApplication::font("QTextEdit"));

    for (auto i=0; i<static_cast<int>(QPageSize::PageSizeId::LastPageSize); i++) {
        const QPageSize ps(static_cast<QPageSize::PageSizeId>(i));
        if (!ps.isValid()) continue;
        if (ps.name().isEmpty()) continue;
        ui->comboTextPageSize->addItem(ps.name(),static_cast<int>(ps.id()));
    }
}

ZSettingsDialog::~ZSettingsDialog()
{
    delete ui;
}

QColor ZSettingsDialog::getBkColor() const
{
    return ui->frameBkColor->palette().brush(QPalette::Window).color();
}

QColor ZSettingsDialog::getFrameColor() const
{
    return ui->frameFrameColor->palette().brush(QPalette::Window).color();
}

QColor ZSettingsDialog::getTextDocBkColor() const
{
    return ui->frameTextBkColor->palette().brush(QPalette::Window).color();
}

QFont ZSettingsDialog::getIdxFont() const
{
    return ui->labelIdxFont->font();
}

QFont ZSettingsDialog::getOCRFont() const
{
    return ui->labelOCRFont->font();
}

QFont ZSettingsDialog::getTextDocFont() const
{
    return ui->labelTextFont->font();
}

QStringList ZSettingsDialog::getIgnoredFiles() const
{
    QStringList res;
    res.reserve(ui->listIgnoredFiles->count());
    for(int i=0;i<ui->listIgnoredFiles->count();i++)
        res << ui->listIgnoredFiles->item(i)->data(Qt::UserRole).toString();
    return res;
}

void ZSettingsDialog::setIgnoredFiles(const QStringList& files) const
{
    ui->listIgnoredFiles->clear();
    for(const QString& fname : files) {
        QFileInfo fi(fname);
        auto *li = new QListWidgetItem(fi.fileName());
        li->setData(Qt::UserRole,fname);
        li->setToolTip(fname);
        ui->listIgnoredFiles->addItem(li);
    }
}

void ZSettingsDialog::setSearchEngines(const ZStrMap &engines) const
{
    ui->listSearch->clear();
    for (auto it = engines.constKeyValueBegin(), end = engines.constKeyValueEnd(); it != end; ++it) {
        auto *li = new QListWidgetItem(QSL("%1 [ %2 ] %3").
                                      arg((*it).first,(*it).second,
                                          (*it).first==zF->global()->getDefaultSearchEngine() ? tr("(default)") : QString()));
        li->setData(Qt::UserRole,(*it).first);
        li->setData(Qt::UserRole+1,(*it).second);
        ui->listSearch->addItem(li);
    }
}

ZStrMap ZSettingsDialog::getSearchEngines() const
{
    ZStrMap engines;
    for (int i=0; i<ui->listSearch->count(); i++) {
        engines[ui->listSearch->item(i)->data(Qt::UserRole).toString()] =
                ui->listSearch->item(i)->data(Qt::UserRole+1).toString();
    }
    return engines;

}

QString ZSettingsDialog::getOCRLanguage() const
{
    return ui->comboOCRLanguage->currentData().toString();
}

QString ZSettingsDialog::getTranSourceLanguage() const
{
    return ui->comboLangSource->currentData().toString();
}

QString ZSettingsDialog::getTranDestLanguage() const
{
    return ui->comboLangDest->currentData().toString();
}

void ZSettingsDialog::updateOCRLanguages() const
{
#ifdef WITH_TESSERACT
    ui->comboOCRLanguage->clear();
    QString selectedLang = ZTesseractOCR::tesseractGetActiveLanguage();

    QDir datapath(ui->editOCRDatapath->text());
    if (datapath.isReadable()) {
        int aidx = -1;
        const QFileInfoList files = datapath.entryInfoList( { QSL("*.traineddata") } );
        for (const QFileInfo& fi : files) {
            QString base = fi.baseName();
            ui->comboOCRLanguage->addItem(fi.fileName(),base);
            if (base==selectedLang)
                aidx = ui->comboOCRLanguage->count()-1;
        }

        if (aidx>=0) {
            ui->comboOCRLanguage->setCurrentIndex(aidx);
        } else if (ui->comboOCRLanguage->count()>0) {
            ui->comboOCRLanguage->setCurrentIndex(0);
        }
    }
#endif
}

void ZSettingsDialog::updateTextPageSizes(int idx) const
{
    bool ok = false;
    auto id = static_cast<QPageSize::PageSizeId>(ui->comboTextPageSize->itemData(idx).toInt(&ok));
    if (!ok) return;

    QPageSize ps;
    if (id == QPageSize::PageSizeId::Custom) {
        ps = QPageSize(QSize(ui->spinTextWidth->value(),
                             ui->spinTextHeight->value()),QString(),QPageSize::ExactMatch);
    } else {
        ps = QPageSize(id);
    }
    if (!ps.isValid()) return;

    ui->spinTextWidth->setEnabled(id == QPageSize::PageSizeId::Custom);
    ui->spinTextHeight->setEnabled(id == QPageSize::PageSizeId::Custom);

    ui->spinTextWidth->setValue(ps.sizePoints().width());
    ui->spinTextHeight->setValue(ps.sizePoints().height());
}

QPageSize ZSettingsDialog::getTextDocPageSize() const
{
    bool ok = false;
    auto id = static_cast<QPageSize::PageSizeId>(ui->comboTextPageSize->currentData().toInt(&ok));

    if (!ok)
        return QPageSize();

    if (id != QPageSize::PageSizeId::Custom)
        return QPageSize(id);

    return QPageSize(QSize(ui->spinTextWidth->value(),
                           ui->spinTextHeight->value()),QString(),QPageSize::ExactMatch);
}

void ZSettingsDialog::setTextDocPageSize(const QPageSize &size) const
{
    int idx = ui->comboTextPageSize->findData(static_cast<int>(size.id()));
    if (idx<0 || idx>=ui->comboTextPageSize->count())
        idx = 0;

    if (size.id() == QPageSize::PageSizeId::Custom) {
        ui->spinTextWidth->setValue(size.sizePoints().width());
        ui->spinTextHeight->setValue(size.sizePoints().height());
    }
    ui->comboTextPageSize->setCurrentIndex(idx);
}

void ZSettingsDialog::updateTranslatorLanguages() const
{
#ifndef Q_OS_WIN
    ui->comboLangSource->clear();
    ui->comboLangDest->clear();
    const QStringList sl = zF->global()->getLanguageCodes();
    int idx1 = -1;
    int idx2 = -1;
    for (const QString& bcp : sl) {
        ui->comboLangSource->addItem(zF->global()->getLanguageName(bcp),QVariant::fromValue(bcp));
        ui->comboLangDest->addItem(zF->global()->getLanguageName(bcp),QVariant::fromValue(bcp));
        if (bcp==zF->global()->getTranSourceLang()) idx1 = ui->comboLangSource->count()-1;
        if (bcp==zF->global()->getTranDestLang()) idx2 = ui->comboLangDest->count()-1;
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

    ZMultiInputDialog dlg(this,tr("Add new search engine"),data,hlp);
    if (dlg.exec()==QDialog::Accepted) {
        data = dlg.getInputData();

        if (getSearchEngines().keys().contains(data[tr("Menu title")])) {
            QMessageBox::warning(this,QGuiApplication::applicationDisplayName(),
                                 tr("Unable to add two or more engines with same names.\n"
                                    "Use another name for new engine."));
        } else {
            auto *li = new QListWidgetItem(QSL("%1 [ %2 ] %3").
                                          arg(data[tr("Menu title")],
                                          data[tr("Url template")],
                    data[tr("Menu title")]==zF->global()->getDefaultSearchEngine() ? tr("(default)") : QString()));
            li->setData(Qt::UserRole,data[tr("Menu title")]);
            li->setData(Qt::UserRole+1,data[tr("Url template")]);
            ui->listSearch->addItem(li);
        }
    }
}

void ZSettingsDialog::delSearchEngine() const
{
    const QList<QListWidgetItem *> dl = ui->listSearch->selectedItems();
    for (QListWidgetItem* i : dl) {
        ui->listSearch->removeItemWidget(i);
        delete i;
    }
}

void ZSettingsDialog::setDefaultSearch() const
{
    QList<QListWidgetItem *> dl = ui->listSearch->selectedItems();
    if (dl.isEmpty()) return;

    zF->global()->setDefaultSearchEngine(dl.first()->data(Qt::UserRole).toString());

    setSearchEngines(getSearchEngines());
}

void ZSettingsDialog::updateSQLFields(bool checked) const
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
    const QString datapath = ZGenericFuncs::getExistingDirectoryD(this,
                                                                  tr("Tesseract datapath"),
                                                                  ui->editOCRDatapath->text());
    if (!datapath.isEmpty())
        ui->editOCRDatapath->setText(datapath);
    updateOCRLanguages();
}

void ZSettingsDialog::gcpKeyFileDlg()
{
    const QString file = ZGenericFuncs::getOpenFileNameD(this,tr("Select GCP service JSON key"),ui->editGCPKeyFile->text(),
                                                         QSL("JSON GCP service access key (*.json)"));
    if (!file.isEmpty())
        ui->editGCPKeyFile->setText(file);
}

void ZSettingsDialog::delListWidgetItem()
{
    auto *const btn = sender();
    if (btn==nullptr) return;
    QListWidget* list = nullptr;
    if (btn->objectName()==ui->btnDeleteBookmark->objectName()) {
        list = ui->listBookmarks;
    } else if (btn->objectName()==ui->btnDynDelete->objectName()) {
        list = ui->listDynAlbums;
    } else if (btn->objectName()==ui->btnDeleteIgnored->objectName()) {
        list = ui->listIgnoredFiles;
    }
    if (list==nullptr) return;

    const QList<QListWidgetItem *> dl = list->selectedItems();
    for (QListWidgetItem* i : dl) {
        list->removeItemWidget(i);
        delete i;
    }
}

void ZSettingsDialog::bkColorDlg()
{
    QColor c = QColorDialog::getColor(getBkColor(),this);
    if (!c.isValid()) return;
    updateBkColor(c);
}

void ZSettingsDialog::idxFontDlg()
{
    bool ok = false;
    QFont f = QFontDialog::getFont(&ok,getIdxFont(),this);
    if (!ok) return;
    updateIdxFont(f);
}

void ZSettingsDialog::ocrFontDlg()
{
    bool ok = false;
    QFont f = QFontDialog::getFont(&ok,getOCRFont(),this);
    if (!ok) return;
    updateOCRFont(f);
}

void ZSettingsDialog::textDocFontDlg()
{
    bool ok = false;
    QFont f = QFontDialog::getFont(&ok,getTextDocFont(),this);
    if (!ok) return;
    updateTextDocFont(f);
}

void ZSettingsDialog::frameColorDlg()
{
    QColor c = QColorDialog::getColor(getFrameColor(),this);
    if (!c.isValid()) return;
    updateFrameColor(c);
}

void ZSettingsDialog::textDocBkColorDlg()
{
    QColor c = QColorDialog::getColor(getTextDocBkColor(),this);
    if (!c.isValid()) return;
    updateTextDocBkColor(c);
}

void ZSettingsDialog::updateBkColor(const QColor &c) const
{
    QPalette p = ui->frameBkColor->palette();
    p.setBrush(QPalette::Window,QBrush(c));
    ui->frameBkColor->setPalette(p);
}

void ZSettingsDialog::updateFrameColor(const QColor &c) const
{
    QPalette p = ui->frameFrameColor->palette();
    p.setBrush(QPalette::Window,QBrush(c));
    ui->frameFrameColor->setPalette(p);
}

void ZSettingsDialog::updateTextDocBkColor(const QColor &c) const
{
    QPalette p = ui->frameTextBkColor->palette();
    p.setBrush(QPalette::Window,QBrush(c));
    ui->frameTextBkColor->setPalette(p);
}

void ZSettingsDialog::updateIdxFont(const QFont &f) const
{
    ui->labelIdxFont->setFont(f);
    ui->labelIdxFont->setText(QSL("%1, %2").arg(f.family()).arg(f.pointSize()));
}

void ZSettingsDialog::updateOCRFont(const QFont &f) const
{
    ui->labelOCRFont->setFont(f);
    ui->labelOCRFont->setText(QSL("%1, %2").arg(f.family()).arg(f.pointSize()));
}

void ZSettingsDialog::updateTextDocFont(const QFont &f) const
{
    ui->labelTextFont->setFont(f);
    ui->labelTextFont->setText(QSL("%1, %2").arg(f.family()).arg(f.pointSize()));
}

void ZSettingsDialog::dynAdd()
{
    ZTwoEditDlg dlg(this,tr("Add dynamic list"),tr("List title"),
                    tr("Query part"));
    dlg.setHelpText(tr("Query part contains part of SELECT query on manga table "
                       "from FROM clause to the end of query.\n"
                       "This part can consists of WHERE, ORDER, LIMIT and any other MySQL SELECT clauses."));

    connect(zF->global()->db(),&ZDB::gotTablesDescription,&dlg,&ZTwoEditDlg::setAuxText,Qt::QueuedConnection);
    connect(this,&ZSettingsDialog::getTablesDescription,
            zF->global()->db(),&ZDB::sqlGetTablesDescription,Qt::QueuedConnection);
    Q_EMIT getTablesDescription();

    if (dlg.exec()==QDialog::Accepted) {
        auto *li = new QListWidgetItem(QSL("%1 [ %2 ]").
                                      arg(dlg.getDlgEdit1(),
                                          dlg.getDlgEdit2()));
        li->setData(Qt::UserRole,dlg.getDlgEdit1());
        li->setData(Qt::UserRole+1,dlg.getDlgEdit2());
        ui->listDynAlbums->addItem(li);
    }
}

void ZSettingsDialog::dynEdit()
{
    if (ui->listDynAlbums->selectedItems().isEmpty()) return;
    ZTwoEditDlg dlg(this,tr("Edit dynamic list"),tr("List title"),
                    tr("Query part"),
                    ui->listDynAlbums->selectedItems().first()->data(Qt::UserRole).toString(),
                    ui->listDynAlbums->selectedItems().first()->data(Qt::UserRole+1).toString());
    dlg.setHelpText(tr("Query part contains part of SELECT query on manga table "
                       "from FROM clause to the end of query.\n"
                       "This part can consists of WHERE, ORDER, LIMIT and any other MySQL SELECT clauses."));
    if (dlg.exec()==QDialog::Accepted) {
        ui->listDynAlbums->selectedItems().first()->setText(QSL("%1 [ %2 ]").
                                                            arg(dlg.getDlgEdit1(),
                                                                dlg.getDlgEdit2()));
        ui->listDynAlbums->selectedItems().first()->setData(Qt::UserRole,dlg.getDlgEdit1());
        ui->listDynAlbums->selectedItems().first()->setData(Qt::UserRole+1,dlg.getDlgEdit2());
    }
}

void ZSettingsDialog::openRar()
{
    QString filter = QSL("*");
#ifdef Q_OS_WIN
    filter = tr("Executable files (*.exe)");
#endif
    const QString filename = ZGenericFuncs::getOpenFileNameD(this,tr("Select console RAR binary"),
                                                             zF->global()->getSavedAuxOpenDir(),filter);
    if (!filename.isEmpty())
        ui->editRar->setText(filename);
}

void ZSettingsDialog::openOffice()
{
    QString filter = QSL("*");
#ifdef Q_OS_WIN
    filter = tr("Executable files (*.exe)");
#endif
    QString filename = ZGenericFuncs::getOpenFileNameD(this,tr("Select LibreOffice headless converter binary"),
                                                       zF->global()->getSavedAuxOpenDir(),filter);
    if (!filename.isEmpty())
        ui->editOffice->setText(filename);
}
