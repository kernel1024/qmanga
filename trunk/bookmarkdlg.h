#ifndef BOOKMARKDLG_H
#define BOOKMARKDLG_H

#include <QDialog>

namespace Ui {
class QBookmarkDlg;
}

class QBookmarkDlg : public QDialog
{
    Q_OBJECT
    
public:
    explicit QBookmarkDlg(QWidget *parent = 0, const QString &name = QString(), const QString &filename = QString());
    ~QBookmarkDlg();
    QString getBkTitle();
    QString getBkFilename();
    
private:
    Ui::QBookmarkDlg *ui;
};

#endif // BOOKMARKDLG_H
