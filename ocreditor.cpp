#include <utility>
#include <QMenu>
#include <QToolTip>
#include <QUrl>
#include <QUrlQuery>
#include <QDesktopServices>
#include <QToolButton>
#include <QDebug>
#include "zglobal.h"
#include "qxttooltip.h"
#include "ocreditor.h"
#include "ui_ocreditor.h"

ZOCREditor::ZOCREditor(QWidget *parent) :
    QDialog(parent, Qt::Tool),
    ui(new Ui::ZOCREditor)
{
    ui->setupUi(this);

#ifdef QT_DBUS_LIB
    translator = new OrgKernel1024JpreaderAuxtranslatorInterface(
                     QSL("org.kernel1024.jpreader"),
                     QSL("/auxTranslator"),
                     QDBusConnection::sessionBus(),this);

    browser = new OrgKernel1024JpreaderBrowsercontrollerInterface(
                  QSL("org.kernel1024.jpreader"),
                  QSL("/browserController"),
                  QDBusConnection::sessionBus(),this);

    dictionary = new OrgQjradDictionaryInterface(QSL("org.qjrad.dictionary"),
                                                 QSL("/"),
                                                 QDBusConnection::sessionBus(),this);
#endif
    ui->editor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->editor,&QPlainTextEdit::customContextMenuRequested,
            this,&ZOCREditor::contextMenu);

    dictSearch = new QAction(QIcon(QSL(":/16/document-preview")),
                             tr("Dictionary search"),this);
    dictSearch->setCheckable(true);
    dictSearch->setChecked(false);
    connect(ui->editor,&QPlainTextEdit::selectionChanged,
            this,&ZOCREditor::selectionChanged);

    ui->dictSearchButton->setDefaultAction(dictSearch);

    selectionTimer = new QTimer(this);
    selectionTimer->setInterval(ZDefaults::oneSecondMS);
    selectionTimer->setSingleShot(true);
    connect(selectionTimer,&QTimer::timeout,this,&ZOCREditor::selectionShow);
    storedSelection.clear();

    connect(ui->translateButton,&QToolButton::clicked,
            this,&ZOCREditor::translate);
#ifdef QT_DBUS_LIB
    connect(translator,&OrgKernel1024JpreaderAuxtranslatorInterface::gotTranslation,
            this,&ZOCREditor::gotTranslation);

    connect(dictionary,&OrgQjradDictionaryInterface::gotWordTranslation,
            this,&ZOCREditor::showDictToolTip);
#else
    ui->groupTranslation->hide();
    ui->translateButton->setEnabled(false);
#endif
    ui->status->clear();
}

ZOCREditor::~ZOCREditor()
{
    delete ui;
}

void ZOCREditor::setEditorFont(const QFont &font)
{
    ui->editor->setFont(font);
    ui->translatedEditor->setFont(font);
}

void ZOCREditor::findWordTranslation(const QString &text)
{
    if (text.isEmpty()) return;
#ifdef QT_DBUS_LIB
    if (!dictionary->isValid()) {
        ui->status->setText(tr("Dictionary not ready."));
    } else {
        dictionary->findWordTranslation(text);
        ui->status->setText(tr("Searching in dictionary..."));
    }
#endif
}

void ZOCREditor::addOCRText(const QStringList &text)
{
    for (const auto &i : text)
        ui->editor->appendPlainText(i);
    showWnd();
}

void ZOCREditor::showWnd()
{
    if (!isVisible())
        move(QCursor::pos());
    show();
}

void ZOCREditor::translate()
{
    ui->status->clear();
#ifdef QT_DBUS_LIB
    if (translator!=nullptr) {
        if (!translator->isValid()) {
            ui->status->setText(tr("Aux translator not ready."));
        } else {
            QString s = ui->editor->toPlainText();
            if (!ui->editor->textCursor().selectedText().isEmpty())
                s = ui->editor->textCursor().selectedText();
            translator->setSrcLang(zF->global()->getTranSourceLang());
            translator->setDestLang(zF->global()->getTranDestLang());
            translator->startAuxTranslation(s);
            ui->status->setText(tr("Translation in progress..."));
        }
    }
#endif
}

void ZOCREditor::gotTranslation(const QString &text)
{
    ui->translatedEditor->appendPlainText(text);
    ui->status->setText(tr("Ready"));
}

void ZOCREditor::contextMenu(const QPoint &pos)
{
    QMenu cm(this);

    QAction *ac = nullptr;

    ac = new QAction(QIcon(QSL(":/16/edit-cut")),tr("Cut"),nullptr);
    connect(ac,&QAction::triggered,ui->editor,&QPlainTextEdit::cut);
    cm.addAction(ac);

    ac = new QAction(QIcon(QSL(":/16/edit-copy")),tr("Copy"),nullptr);
    connect(ac,&QAction::triggered,ui->editor,&QPlainTextEdit::copy);
    cm.addAction(ac);

    ac = new QAction(QIcon(QSL(":/16/edit-paste")),tr("Paste"),nullptr);
    connect(ac,&QAction::triggered,ui->editor,&QPlainTextEdit::paste);
    cm.addAction(ac);

    ac = new QAction(QIcon(QSL(":/16/edit-clear")),tr("Clear"),nullptr);
    connect(ac,&QAction::triggered,ui->editor,&QPlainTextEdit::clear);
    cm.addAction(ac);

    QString sText;
    if (ui->editor->textCursor().hasSelection())
        sText = ui->editor->textCursor().selectedText();

    if (!sText.isEmpty()) {
        cm.addSeparator();

#ifdef QT_DBUS_LIB
        cm.addAction(dictSearch);

        ac = new QAction(QIcon(QSL(":/16/accessories-dictionary")),
                         tr("Show qjrad window"),nullptr);
        connect(ac,&QAction::triggered,this,[this,sText](){
            if (dictionary->isValid())
                dictionary->showDictionaryWindow(sText);
        });
        cm.addAction(ac);

        ac = new QAction(QIcon(QSL(":/16/jpreader")),
                         tr("Search with jpreader"),nullptr);
        connect(ac,&QAction::triggered,this,[this,sText](){
            if (browser->isValid()) {
                if (sText.trimmed().startsWith(QSL("http"),Qt::CaseInsensitive)) {
                    QUrl u = QUrl::fromUserInput(sText);
                    if (u.isValid())
                        browser->openUrl(u.toString());
                } else
                    browser->openDefaultSearch(sText);
            }
        });
        cm.addAction(ac);
#endif

        if (!zF->global()->getCtxSearchEngines().isEmpty()) {
            cm.addSeparator();

            QStringList searchNames = zF->global()->getCtxSearchEngines().keys();
            searchNames.sort(Qt::CaseInsensitive);
            for (const QString& name : std::as_const(searchNames)) {
                QUrl url = zF->global()->createSearchUrl(sText,name);
                if (!url.isEmpty() && url.isValid()) {
                    ac = new QAction(name,nullptr);
                    connect(ac, &QAction::triggered, [url](){
                        QDesktopServices::openUrl(url);
                    });
                    cm.addAction(ac);
                }
            }
        }
    }

    cm.exec(ui->editor->mapToGlobal(pos));
}

void ZOCREditor::selectionChanged()
{
    storedSelection = ui->editor->textCursor().selectedText();

#ifdef QT_DBUS_LIB
    if (dictSearch->isChecked() && !storedSelection.isEmpty() && dictionary!=nullptr)
        selectionTimer->start();
#endif
}

void ZOCREditor::selectionShow()
{
#ifdef QT_DBUS_LIB
    if (dictionary==nullptr || storedSelection.isEmpty()) return;

    findWordTranslation(storedSelection);
#endif
}

void ZOCREditor::showDictToolTip(const QString &html)
{
    qDebug() << html;
    ui->status->setText(tr("Ready"));

    auto *t = new QLabel(html);
    t->setOpenExternalLinks(false);
    t->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::TextSelectableByMouse);
    t->setMaximumSize(ZDefaults::maxDictTooltipSize);
    t->setStyleSheet(QSL("QLabel { background: #fefdeb; }"));

    connect(t,&QLabel::linkActivated,this,&ZOCREditor::showSuggestedTranslation);

    QxtToolTip::show(QCursor::pos(),t,ui->editor,QRect(),true,true);
}

void ZOCREditor::showSuggestedTranslation(const QString &link)
{
    QUrl url(link);
    QUrlQuery requ(url);
    QString word = requ.queryItemValue(QSL("word"));
    if (word.startsWith(u'%')) {
        QByteArray bword = word.toLatin1();
        if (!bword.isNull() && !bword.isEmpty())
            word = QUrl::fromPercentEncoding(bword);
    }
    findWordTranslation(word);
}
