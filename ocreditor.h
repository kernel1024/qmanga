#ifndef OCREDITOR_H
#define OCREDITOR_H

#include <QDialog>
#include <QAction>
#include <QTimer>
#include "auxtranslator_interface.h"
#include "dictionary_interface.h"

namespace Ui {
class ZOCREditor;
}

class ZOCREditor : public QDialog
{
    Q_OBJECT
    
public:
    explicit ZOCREditor(QWidget *parent = 0);
    ~ZOCREditor();
    void addText(const QStringList& text);
    
private:
    Ui::ZOCREditor *ui;
    OrgJpreaderAuxtranslatorInterface* translator;
    OrgQjradDictionaryInterface* dictionary;
    QAction* dictSearch;
    QTimer* selectionTimer;
    QString storedSelection;
    void findWordTranslation(const QString& text);

public slots:
    void showWnd();
    void translate();
    void gotTranslation(const QString& text);
    void contextMenu(const QPoint& pos);
    void selectionChanged();
    void selectionShow();
    void showDictToolTip(const QString& html);
    void showSuggestedTranslation(const QString& link);
};

#endif // OCREDITOR_H
