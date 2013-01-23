#include <QResizeEvent>
#include "zscrollarea.h"

ZScrollArea::ZScrollArea(QWidget *parent) :
    QScrollArea(parent)
{
}

void ZScrollArea::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);
    emit sizeChanged(event->size());
}

ZTabWidget::ZTabWidget(QWidget *parent) :
    QTabWidget(parent)
{
}

QTabBar *ZTabWidget::tabBar() const
{
    return QTabWidget::tabBar();
}
