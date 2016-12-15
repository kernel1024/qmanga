#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QWidget>
#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QListWidget>
#include <QRadioButton>
#include <QCheckBox>
#include <QLabel>

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
    QLineEdit* editMySqlHost;
    QLineEdit* editRar;
    QListWidget* listBookmarks;
    QRadioButton* radioCachePixmaps;
    QRadioButton* radioCacheData;
    QCheckBox* checkFineRendering;
    QCheckBox* checkFSWatcher;
    QSpinBox* spinScrollDelta;
    QLabel* labelDetectedDelta;
    QListWidget* listDynAlbums;
    QComboBox* comboPDFRendererMode;
    QCheckBox* checkForceDPI;
    QDoubleSpinBox* spinForceDPI;

    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();
    QColor getBkColor();
    QFont getIdxFont();
    QFont getOCRFont();
    QColor getFrameColor();
    QStringList getIgnoredFiles();
    void setIgnoredFiles(const QStringList &files);
    
private:
    QColor bkColor, frameColor;
    QFont idxFont;
    QFont ocrFont;
    Ui::SettingsDialog *ui;
    QMap<QPushButton *, QListWidget *> delLookup;

public slots:
    void delListWidgetItem();
    void bkColorDlg();
    void idxFontDlg();
    void ocrFontDlg();
    void frameColorDlg();
    void updateBkColor(QColor c);
    void updateIdxFont(QFont f);
    void updateOCRFont(QFont f);
    void updateFrameColor(QColor c);
    void dynAdd();
    void dynEdit();
    void openRar();

};

#endif // SETTINGSDIALOG_H
