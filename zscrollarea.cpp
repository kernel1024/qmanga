#include <QResizeEvent>
#include "zscrollarea.h"
#include "zglobal.h"

ZScrollArea::ZScrollArea(QWidget *parent) :
    QScrollArea(parent)
{
    resizeTimer.setInterval(ZDefaults::resizeTimerInitialMS);
    resizeTimer.setSingleShot(true);
    connect(&resizeTimer,&QTimer::timeout,this,[this](){
        if (savedSize.isValid())
            Q_EMIT sizeChanged(savedSize);
    });
}

void ZScrollArea::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);
    savedSize = event->size();

    int preferredInterval = static_cast<int>(zF->global()->getAvgFineRenderTime()) / 2;
    if ((qAbs(preferredInterval - resizeTimer.interval()) > ZDefaults::resizeTimerDiffMS)
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
