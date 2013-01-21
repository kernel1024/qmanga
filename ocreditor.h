#ifndef OCREDITOR_H
#define OCREDITOR_H

#include <QDialog>

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

public slots:
    void showWnd();
};

#endif // OCREDITOR_H
