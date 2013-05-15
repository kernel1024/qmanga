#ifndef ALBUMSELECTORDLG_H
#define ALBUMSELECTORDLG_H

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

    explicit AlbumSelectorDlg(QWidget *parent, QStringList albums, QString suggest, int toAddCount);
    ~AlbumSelectorDlg();
    
private:
    Ui::AlbumSelectorDlg *ui;
};

#endif // ALBUMSELECTORDLG_H
