#include <QLineEdit>

#include "albumselectordlg.h"
#include "ui_albumselectordlg.h"

AlbumSelectorDlg::AlbumSelectorDlg(QWidget *parent, QStringList albums, QString suggest, int toAddCount) :
    QDialog(parent),
    ui(new Ui::AlbumSelectorDlg)
{
    ui->setupUi(this);

    listAlbums = ui->listAlbums;

    if (toAddCount<0)
        ui->label->setText(tr("Add manga to album"));
    else
        ui->label->setText(tr("Add %1 files to album").arg(toAddCount));

    ui->listAlbums->clear();
    ui->listAlbums->addItems(albums);
    ui->listAlbums->lineEdit()->setText(suggest);
    int idx = albums.indexOf(QRegExp(suggest,Qt::CaseInsensitive));
    if (idx>=0)
        ui->listAlbums->setCurrentIndex(idx);
}

AlbumSelectorDlg::~AlbumSelectorDlg()
{
    delete ui;
}
