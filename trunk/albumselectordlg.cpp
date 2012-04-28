#include "albumselectordlg.h"
#include "ui_albumselectordlg.h"

AlbumSelectorDlg::AlbumSelectorDlg(QWidget *parent, QStringList albums, QString suggest) :
    QDialog(parent),
    ui(new Ui::AlbumSelectorDlg)
{
    ui->setupUi(this);

    listAlbums = ui->listAlbums;

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