#include "zsearchtab.h"
#include "ui_zsearchtab.h"
#include "zglobal.h"
#include "zmangamodel.h"

ZSearchTab::ZSearchTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ZSearchTab)
{
    ui->setupUi(this);

    ui->srcIconSize->setMinimum(16);
    ui->srcIconSize->setMaximum(maxPreviewSize);
    ui->srcIconSize->setValue(128);

    connect(ui->srcAlbums,SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this,SLOT(albumChanged(QListWidgetItem*,QListWidgetItem*)));
    connect(ui->srcList,SIGNAL(clicked(QModelIndex)),this,SLOT(mangaClicked(QModelIndex)));
    connect(ui->srcList,SIGNAL(activated(QModelIndex)),this,SLOT(mangaOpen(QModelIndex)));

}

ZSearchTab::~ZSearchTab()
{
    delete ui;
}

void ZSearchTab::updateAlbumsList()
{
    ui->srcAlbums->clear();
    ui->srcAlbums->addItems(zGlobal->sqlGetAlbums());
}

void ZSearchTab::albumChanged(QListWidgetItem *current, QListWidgetItem *)
{
    if (current==NULL) return;

    mangaList = zGlobal->sqlGetFiles(current->text(),ZGlobal::smName,false);

    QItemSelectionModel *m = ui->srcList->selectionModel();
    QAbstractItemModel *n = ui->srcList->model();
    ui->srcList->setModel(new ZMangaModel(this,&mangaList,ui->srcIconSize->value()));
    m->deleteLater();
    n->deleteLater();
    ui->srcDesc->clear();
}

void ZSearchTab::mangaClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    if (index.row()>=mangaList.length()) return;
    ui->srcDesc->clear();
    QString msg;

/*    msg = tr("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">");
    msg += tr("<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">");
    msg += tr("p, li { white-space: pre-wrap; }");
    msg += tr("</style></head><body style=\" font-family:'%1'; font-size:%2pt; font-weight:400; font-style:normal;\">").
            arg(QApplication::font("QTextBrowser").family()).
            arg(QApplication::font("QTextBrowser").pointSize());
    msg += tr("<p style=\" margin-top:0px; margin-bottom:10px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'%1'; font-size:36pt;\">%2</span></p>").arg(fontResults.family()).arg(ki.kanji);

    msg += tr("<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Strokes:</span> %1</p>").arg(ki.strokes);
    msg += tr("<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Parts:</span> %1</p>").arg(ki.parts.join(tr(" ")));
    msg += tr("<p style=\" margin-top:0px; margin-bottom:10px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Grade:</span> %1</p>").arg(ki.grade);

    msg += tr("<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">On:</span> %1</p>").arg(ki.onReading.join(tr(", ")));
    msg += tr("<p style=\" margin-top:0px; margin-bottom:10px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Kun:</span> %1</p>").arg(ki.kunReading.join(tr(", ")));

    msg += tr("<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Meaning:</span> %1</p>").arg(ki.meaning.join(tr(", ")));
    msg += tr("</body></html>");*/

    ui->srcDesc->setHtml(msg);
}

void ZSearchTab::mangaOpen(const QModelIndex &index)
{
    if (!index.isValid()) return;
    if (index.row()>=mangaList.length()) return;
    emit mangaDblClick(mangaList.at(index.row()).filename);
}

void ZSearchTab::mangaAdd()
{
    QStringList fl = zGlobal->getOpenFileNamesD(this,tr("Add manga to index"),zGlobal->savedIndexOpenDir);
    if (fl.isEmpty()) return;
    QFileInfo fi(fl.first());
    zGlobal->savedIndexOpenDir = fi.path();

    //...
}

void ZSearchTab::mangaDel()
{
}
