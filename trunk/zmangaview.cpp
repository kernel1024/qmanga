#include "zmangaview.h"

ZMangaView::ZMangaView(QWidget *parent) :
    QGraphicsView(parent)
{
    iList = NULL;
    currentPage = 0;
}

void ZMangaView::setImageList(QImageList *ilist)
{
    iList = ilist;
    currentPage = 0;
    setPage(0);
}

void ZMangaView::setPage(int page)
{
    if (!iList) return;
    if (page<0 || page>=iList->count()) return;

    if (!scene()) setScene(new QGraphicsScene(this));
    scene()->clear();
    scene()->addPixmap(QPixmap::fromImage(iList->at(page)));
//    scene()->addLine(0.0,0.0,500.0,500.0);

    currentPage = page;
    emit loadPage(page);
}

void ZMangaView::wheelEvent(QWheelEvent *event)
{
    int numDegrees = event->delta() / 8;
    int numSteps = numDegrees / 15;

    setPage(currentPage-numSteps);
    event->accept();
}
