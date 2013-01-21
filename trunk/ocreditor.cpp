#include "ocreditor.h"
#include "ui_ocreditor.h"

ZOCREditor::ZOCREditor(QWidget *parent) :
    QDialog(parent, Qt::Tool),
    //CustomizeWindowHint | Qt::WindowTitleHint ),
    ui(new Ui::ZOCREditor)
{
    ui->setupUi(this);
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
