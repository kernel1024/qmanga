#ifndef ALBUMSELECTORDLG_H
#define ALBUMSELECTORDLG_H

#include <QtCore>
#include <QDialog>
#include <QComboBox>

namespace Ui {
class AlbumSelectorDlg;
}

class AlbumSelectorDlg : public QDialog
{
    Q_OBJECT
    
public:
    QComboBox* listAlbums;

    explicit AlbumSelectorDlg(QWidget *parent, QStringList albums, QString suggest);
    ~AlbumSelectorDlg();
    
private:
    Ui::AlbumSelectorDlg *ui;
};

#endif // ALBUMSELECTORDLG_H
