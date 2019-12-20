#ifndef ALBUMSELECTORDLG_H
#define ALBUMSELECTORDLG_H

#include <QDialog>
#include <QComboBox>

namespace Ui {
class ZAlbumSelectorDlg;
}

class ZAlbumSelectorDlg : public QDialog
{
    Q_OBJECT
    
public:
    explicit ZAlbumSelectorDlg(QWidget *parent, const QStringList &albums,
                              const QString &suggest, int toAddCount);
    ~ZAlbumSelectorDlg() override;
    QString getAlbumName();
    
private:
    Ui::ZAlbumSelectorDlg *ui;

    Q_DISABLE_COPY(ZAlbumSelectorDlg)
};

#endif // ALBUMSELECTORDLG_H
