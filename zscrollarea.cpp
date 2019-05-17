#include <QResizeEvent>
#include "zscrollarea.h"
#include "zglobal.h"

ZScrollArea::ZScrollArea(QWidget *parent) :
    QScrollArea(parent)
{
    resizeTimer.setInterval(500);
    resizeTimer.setSingleShot(true);
    connect(&resizeTimer,&QTimer::timeout,this,[this](){
        if (savedSize.isValid())
            emit sizeChanged(savedSize);

    });
}

void ZScrollArea::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);
    savedSize = event->size();

    int preferredInterval = static_cast<int>(zg->getAvgFineRenderTime()) / 2;
    if ((qAbs(preferredInterval - resizeTimer.interval()) > 200)
            && (preferredInterval > 100))
        resizeTimer.setInterval(preferredInterval);

    resizeTimer.start();
}

ZTabWidget::ZTabWidget(QWidget *parent) :
    QTabWidget(parent)
{
}

QTabBar *ZTabWidget::tabBar() const
{
    return QTabWidget::tabBar();
}
