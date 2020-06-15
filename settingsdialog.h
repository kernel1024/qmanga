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
class ZSettingsDialog;
}

class ZSettingsDialog : public QDialog
{
    Q_OBJECT
public:
    Ui::ZSettingsDialog *ui;

    explicit ZSettingsDialog(QWidget *parent = nullptr);
    ~ZSettingsDialog() override;
    QColor getBkColor() const;
    QFont getIdxFont() const;
    QFont getOCRFont() const;
    QColor getFrameColor() const;
    QStringList getIgnoredFiles() const;
    void setIgnoredFiles(const QStringList &files) const;
    void setSearchEngines(const ZStrMap &engines) const;
    ZStrMap getSearchEngines() const;
    QString getOCRLanguage() const;
    QString getTranSourceLanguage() const;
    QString getTranDestLanguage() const;
    void updateTranslatorLanguages() const;

Q_SIGNALS:
    void getTablesDescription();

public Q_SLOTS:
    void delListWidgetItem();
    void bkColorDlg();
    void idxFontDlg();
    void ocrFontDlg();
    void frameColorDlg();
    void updateBkColor(const QColor &c) const;
    void updateIdxFont(const QFont &f) const;
    void updateOCRFont(const QFont &f) const;
    void updateFrameColor(const QColor &c) const;
    void dynAdd();
    void dynEdit();
    void openRar(); 
    void addSearchEngine();
    void delSearchEngine() const;
    void setDefaultSearch() const;
    void updateSQLFields(bool checked) const;
    void ocrDatapathDlg();
    void updateOCRLanguages() const;

private:
    Q_DISABLE_COPY(ZSettingsDialog)

};

#endif // SETTINGSDIALOG_H
