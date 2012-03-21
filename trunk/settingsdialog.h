#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QtCore>
#include <QtGui>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT
    
public:
    QComboBox* comboFilter;
    QSpinBox* spinCacheWidth;
    QSpinBox* spinMagnify;
    QLineEdit* editMySqlLogin;
    QLineEdit* editMySqlPassword;
    QListWidget* listBookmarks;

    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();
    
private:
    Ui::SettingsDialog *ui;

public slots:
    void delBookmark();

};

#endif // SETTINGSDIALOG_H
