#include <QMenu>
#include <QToolTip>
#include <QUrl>
#include <QUrlQuery>
#include "qxttooltip.h"
#include "ocreditor.h"
#include "ui_ocreditor.h"

ZOCREditor::ZOCREditor(QWidget *parent) :
    QDialog(parent, Qt::Tool),
    ui(new Ui::ZOCREditor)
{
    ui->setupUi(this);

#ifdef QT_DBUS_LIB
    translator = new OrgJpreaderAuxtranslatorInterface("org.jpreader.auxtranslator","/",
                                                       QDBusConnection::sessionBus(),this);

    dictionary = new OrgQjradDictionaryInterface("org.qjrad.dictionary","/",
                                                 QDBusConnection::sessionBus(),this);
#endif
    ui->editor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->editor,&QPlainTextEdit::customContextMenuRequested,
            this,&ZOCREditor::contextMenu);

    dictSearch = new QAction(QIcon(":/img/16x16/document-preview"),tr("Dictionary search"),this);
    dictSearch->setCheckable(true);
    dictSearch->setChecked(false);
    connect(ui->editor,&QPlainTextEdit::selectionChanged,
            this,&ZOCREditor::selectionChanged);

    selectionTimer = new QTimer(this);
    selectionTimer->setInterval(1000);
    selectionTimer->setSingleShot(true);
    connect(selectionTimer,&QTimer::timeout,this,&ZOCREditor::selectionShow);
    storedSelection.clear();

    connect(ui->translateButton,&QToolButton::clicked,
            this,&ZOCREditor::translate);
#ifdef QT_DBUS_LIB
    connect(translator,&OrgJpreaderAuxtranslatorInterface::gotTranslation,
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

void ZOCREditor::addText(const QStringList &text)
{
    for (int i=0;i<text.count();i++)
        ui->editor->appendPlainText(text.at(i));
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
    if (!dictionary->isValid())
        ui->status->setText(tr("Dictionary not ready."));
    else {
        dictionary->findWordTranslation(text);
        ui->status->setText(tr("Searching in dictionary..."));
    }
#endif
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
    if (translator!=NULL) {
        if (!translator->isValid())
            ui->status->setText(tr("Aux translator not ready."));
        else {
            QString s = ui->editor->toPlainText();
            if (!ui->editor->textCursor().selectedText().isEmpty())
                s = ui->editor->textCursor().selectedText();
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

    QAction *ac;

    ac = new QAction(QIcon(":/img/16x16/edit-cut"),tr("Cut"),NULL);
    connect(ac,&QAction::triggered,ui->editor,&QPlainTextEdit::cut);
    cm.addAction(ac);

    ac = new QAction(QIcon(":/img/16x16/edit-copy"),tr("Copy"),NULL);
    connect(ac,&QAction::triggered,ui->editor,&QPlainTextEdit::copy);
    cm.addAction(ac);

    ac = new QAction(QIcon(":/img/16x16/edit-paste"),tr("Paste"),NULL);
    connect(ac,&QAction::triggered,ui->editor,&QPlainTextEdit::paste);
    cm.addAction(ac);

    ac = new QAction(QIcon(":/img/16x16/edit-clear"),tr("Clear"),NULL);
    connect(ac,&QAction::triggered,ui->editor,&QPlainTextEdit::clear);
    cm.addAction(ac);

    if (ui->editor->textCursor().hasSelection()) {
        cm.addSeparator();

#ifdef QT_DBUS_LIB
        cm.addAction(dictSearch);

        ac = new QAction(QIcon(":/img/16x16/accessories-dictionary"),tr("Show qjrad window"),NULL);
        connect(ac,&QAction::triggered,[this](){
            if (dictionary->isValid())
                dictionary->showDictionaryWindow(ui->editor->textCursor().selectedText());
        });
        cm.addAction(ac);
#endif
    }

    cm.exec(ui->editor->mapToGlobal(pos));
}

void ZOCREditor::selectionChanged()
{
    storedSelection = ui->editor->textCursor().selectedText();

#ifdef QT_DBUS_LIB
    if (dictSearch->isChecked() && !storedSelection.isEmpty() && dictionary!=NULL)
        selectionTimer->start();
#endif
}

void ZOCREditor::selectionShow()
{
#ifdef QT_DBUS_LIB
    if (dictionary==NULL || storedSelection.isEmpty()) return;

    findWordTranslation(storedSelection);
#endif
}

void ZOCREditor::showDictToolTip(const QString &html)
{
    ui->status->setText(tr("Ready"));

    QLabel *t = new QLabel(html);
    t->setOpenExternalLinks(false);
    t->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::TextSelectableByMouse);
    t->setMaximumSize(350,350);
    t->setStyleSheet("QLabel { background: #fefdeb; }");

    connect(t,&QLabel::linkActivated,this,&ZOCREditor::showSuggestedTranslation);

    QxtToolTip::show(QCursor::pos(),t,ui->editor,QRect(),true);
}

void ZOCREditor::showSuggestedTranslation(const QString &link)
{
    QUrl url(link);
    QUrlQuery requ(url);
    QString word = requ.queryItemValue("word");
    if (word.startsWith('%')) {
        QByteArray bword = word.toLatin1();
        if (!bword.isNull() && !bword.isEmpty())
            word = QUrl::fromPercentEncoding(bword);
    }
    findWordTranslation(word);
}
