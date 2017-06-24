#ifndef OCREDITOR_H
#define OCREDITOR_H

#include <QDialog>
#include <QAction>
#include <QTimer>

#ifdef QT_DBUS_LIB
#include "auxtranslator_interface.h"
#include "browsercontroller_interface.h"
#include "dictionary_interface.h"
#endif

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
    void setEditorFont(const QFont& font);
    
private:
    Ui::ZOCREditor *ui;
#ifdef QT_DBUS_LIB
    OrgKernel1024JpreaderAuxtranslatorInterface* translator;
    OrgKernel1024JpreaderBrowsercontrollerInterface* browser;
    OrgQjradDictionaryInterface* dictionary;
#endif
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
