#ifndef MULTIINPUTDIALOG_H
#define MULTIINPUTDIALOG_H

#include <QDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QStringList>
#include "zglobal.h"

namespace Ui {
class ZMultiInputDialog;
}

class ZMultiInputDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ZMultiInputDialog(QWidget *parent, const QString &title,
                               const ZStrMap& data, const QString &helperText = QString());
    ~ZMultiInputDialog() override;

    ZStrMap getInputData() const;

private:
    Ui::ZMultiInputDialog *ui;

    QFormLayout *formLayout;
    QList<QLabel *> labels;
    QList<QLineEdit *> edits;

    Q_DISABLE_COPY(ZMultiInputDialog)
};

#endif // MULTIINPUTDIALOG_H
