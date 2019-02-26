#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QWidget>
#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QListWidget>
#include <QRadioButton>
#include <QCheckBox>
#include <QLabel>

#include "zglobal.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT
    
public:
    QComboBox *comboUpscaleFilter, *comboDownscaleFilter;
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
    QSpinBox* spinScrollFactor;
    QLabel* labelDetectedDelta;
    QListWidget* listDynAlbums;
    QComboBox* comboPDFRendererMode;
    QCheckBox* checkForceDPI;
    QDoubleSpinBox* spinForceDPI;
    QDoubleSpinBox* spinBlur;
    QRadioButton* radioMySQL;
    QRadioButton* radioSQLite;
    QDoubleSpinBox* spinSearchScrollFactor;
    QLineEdit* editOCRDatapath;

    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();
    QColor getBkColor();
    QFont getIdxFont();
    QFont getOCRFont();
    QColor getFrameColor();
    QStringList getIgnoredFiles();
    void setIgnoredFiles(const QStringList &files);
    void setSearchEngines(const ZStrMap &engines);
    ZStrMap getSearchEngines() const;
    QString getOCRLanguage();
    QString getTranSourceLanguage();
    QString getTranDestLanguage();
    
private:
    QColor bkColor, frameColor;
    QFont idxFont;
    QFont ocrFont;
    Ui::SettingsDialog *ui;
    QMap<QPushButton *, QListWidget *> delLookup;

    void updateOCRLanguages();
    void updateTranslatorLanguages();

signals:
    void getTablesDescription();

public slots:
    void delListWidgetItem();
    void bkColorDlg();
    void idxFontDlg();
    void ocrFontDlg();
    void frameColorDlg();
    void updateBkColor(const QColor &c);
    void updateIdxFont(const QFont &f);
    void updateOCRFont(const QFont &f);
    void updateFrameColor(const QColor &c);
    void dynAdd();
    void dynEdit();
    void openRar(); 
    void addSearchEngine();
    void delSearchEngine();
    void setDefaultSearch();
    void updateSQLFields(bool checked);
    void ocrDatapathDlg();
};

#endif // SETTINGSDIALOG_H
