#ifndef MULTIINPUTDIALOG_H
#define MULTIINPUTDIALOG_H

#include <QDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QStringList>
#include "zglobal.h"

namespace Ui {
class CMultiInputDialog;
}

class CMultiInputDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CMultiInputDialog(QWidget *parent, const QString &title,
                               const ZStrMap& data, const QString &helperText = QString());
    ~CMultiInputDialog();

    ZStrMap getInputData() const;

private:
    Ui::CMultiInputDialog *ui;

    QFormLayout *formLayout;
    QList<QLabel *> labels;
    QList<QLineEdit *> edits;
};

#endif // MULTIINPUTDIALOG_H
