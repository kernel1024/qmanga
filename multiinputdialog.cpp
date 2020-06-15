#include "multiinputdialog.h"
#include "ui_multiinputdialog.h"

ZMultiInputDialog::ZMultiInputDialog(QWidget *parent, const QString& title,
                                     const ZStrMap &data, const QString& helperText) :
    QDialog(parent),
    ui(new Ui::ZMultiInputDialog)
{
    ui->setupUi(this);

    setWindowTitle(title);

    formLayout = new QFormLayout();
    formLayout->setObjectName(QSL("formLayout"));

    int i = 0;
    for (auto it = data.constKeyValueBegin(), end = data.constKeyValueEnd(); it != end; ++it) {
        auto *label = new QLabel(this);
        label->setObjectName(QSL("label_%1").arg(i));
        label->setText((*it).first);
        labels << label;

        formLayout->setWidget(i, QFormLayout::LabelRole, label);

        auto *lineEdit = new QLineEdit(this);
        lineEdit->setObjectName(QSL("lineEdit_%1").arg(i));
        lineEdit->setText((*it).second);
        edits << lineEdit;

        formLayout->setWidget(i, QFormLayout::FieldRole, lineEdit);

        i++;
    }

    ui->verticalLayout->insertLayout(0,formLayout);

    if (!helperText.isEmpty()) {
        auto *hlp = new QLabel(this);
        hlp->setObjectName(QSL("label_helper"));
        hlp->setText(helperText);
        ui->verticalLayout->insertWidget(0,hlp);
    }
}

ZMultiInputDialog::~ZMultiInputDialog()
{
    delete ui;
}

ZStrMap ZMultiInputDialog::getInputData() const
{
    ZStrMap res;
    for (int i=0;i<edits.count();i++)
        res[labels.at(i)->text()] = edits.at(i)->text();
    return res;
}
