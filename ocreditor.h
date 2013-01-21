#ifndef OCREDITOR_H
#define OCREDITOR_H

#include <QDialog>
#include "auxtranslator_interface.h"

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

public slots:
    void showWnd();
    void translate();
    void gotTranslation(const QString& text);
};

#endif // OCREDITOR_H
