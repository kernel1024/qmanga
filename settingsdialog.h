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
    QLineEdit* editMySqlBase;
    QListWidget* listBookmarks;

    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();
    QColor getBkColor();
    QFont getIdxFont();
    
private:
    QColor bkColor;
    QFont idxFont;
    Ui::SettingsDialog *ui;

public slots:
    void delBookmark();
    void bkColorDlg();
    void idxFontDlg();
    void updateBkColor(QColor c);
    void updateIdxFont(QFont f);

};

#endif // SETTINGSDIALOG_H