#include <QLineEdit>
#include <QRegularExpression>

#include "albumselectordlg.h"
#include "ui_albumselectordlg.h"

ZAlbumSelectorDlg::ZAlbumSelectorDlg(QWidget *parent, const QStringList &albums,
                                   const QString &suggest, int toAddCount) :
    QDialog(parent),
    ui(new Ui::ZAlbumSelectorDlg)
{
    ui->setupUi(this);

    if (toAddCount<0) {
        ui->label->setText(tr("Add manga to album"));
    } else {
        ui->label->setText(tr("Add %1 files to album").arg(toAddCount));
    }

    ui->listAlbums->clear();
    ui->listAlbums->addItems(albums);
    ui->listAlbums->lineEdit()->setText(suggest);
    int idx = albums.indexOf(QRegularExpression(suggest));
    if (idx>=0)
        ui->listAlbums->setCurrentIndex(idx);
}

ZAlbumSelectorDlg::~ZAlbumSelectorDlg()
{
    delete ui;
}

QString ZAlbumSelectorDlg::getAlbumName()
{
    return ui->listAlbums->lineEdit()->text();
}
