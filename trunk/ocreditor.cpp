#include "ocreditor.h"
#include "ui_ocreditor.h"

ZOCREditor::ZOCREditor(QWidget *parent) :
    QDialog(parent, Qt::Tool),
    ui(new Ui::ZOCREditor)
{
    ui->setupUi(this);
    translator = new OrgJpreaderAuxtranslatorInterface("org.jpreader.auxtranslator","/",
                                                       QDBusConnection::sessionBus(),this);

    connect(translator,SIGNAL(gotTranslation(QString)),this,SLOT(gotTranslation(QString)));
    connect(ui->translateButton,SIGNAL(clicked()),this,SLOT(translate()));

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

void ZOCREditor::showWnd()
{
    if (!isVisible())
        move(QCursor::pos());
    show();
}

void ZOCREditor::translate()
{
    ui->status->clear();
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
}

void ZOCREditor::gotTranslation(const QString &text)
{
    ui->translatedEditor->appendPlainText(text);
    ui->status->setText(tr("Ready"));
}
