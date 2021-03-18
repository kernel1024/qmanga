#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QPageSize>

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
    QFont getIdxFont() const;
    QFont getOCRFont() const;
    QFont getTextDocFont() const;
    QColor getBkColor() const;
    QColor getFrameColor() const;
    QColor getTextDocBkColor() const;
    QStringList getIgnoredFiles() const;
    void setIgnoredFiles(const QStringList &files) const;
    void setSearchEngines(const ZStrMap &engines) const;
    ZStrMap getSearchEngines() const;
    QString getOCRLanguage() const;
    QString getTranSourceLanguage() const;
    QString getTranDestLanguage() const;
    void updateTranslatorLanguages() const;
    QPageSize getTextDocPageSize() const;
    void setTextDocPageSize(const QPageSize& size) const;

Q_SIGNALS:
    void getTablesDescription();

public Q_SLOTS:
    void delListWidgetItem();
    void idxFontDlg();
    void ocrFontDlg();
    void textDocFontDlg();
    void bkColorDlg();
    void frameColorDlg();
    void textDocBkColorDlg();
    void updateIdxFont(const QFont &f) const;
    void updateOCRFont(const QFont &f) const;
    void updateTextDocFont(const QFont &f) const;
    void updateBkColor(const QColor &c) const;
    void updateFrameColor(const QColor &c) const;
    void updateTextDocBkColor(const QColor &c) const;
    void dynAdd();
    void dynEdit();
    void openRar(); 
    void addSearchEngine();
    void delSearchEngine() const;
    void setDefaultSearch() const;
    void updateSQLFields(bool checked) const;
    void ocrDatapathDlg();
    void updateOCRLanguages() const;
    void updateTextPageSizes(int idx) const;

private:
    Q_DISABLE_COPY(ZSettingsDialog)

};

#endif // SETTINGSDIALOG_H
