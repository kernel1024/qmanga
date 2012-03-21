#ifndef ZSCROLLAREA_H
#define ZSCROLLAREA_H

#include <QtCore>
#include <QtGui>

class ZScrollArea : public QScrollArea
{
    Q_OBJECT
public:
    explicit ZScrollArea(QWidget *parent = 0);
    
signals:
    void sizeChanged(const QSize& size);

protected:
    void resizeEvent(QResizeEvent * event);
    
};

class ZTabWidget : public QTabWidget
{
    Q_OBJECT
public:
    ZTabWidget(QWidget *parent = 0);
    QTabBar *tabBar() const;
};

#endif // ZSCROLLAREA_H
