#ifndef ZSCROLLAREA_H
#define ZSCROLLAREA_H

#include <QScrollArea>
#include <QTabWidget>
#include <QTabBar>
#include <QTimer>

class ZScrollArea : public QScrollArea
{
    Q_OBJECT
private:
    QTimer resizeTimer;
    QSize savedSize;

public:
    explicit ZScrollArea(QWidget *parent = nullptr);
    
signals:
    void sizeChanged(const QSize& size);

protected:
    void resizeEvent(QResizeEvent * event);
    
};

class ZTabWidget : public QTabWidget
{
    Q_OBJECT
public:
    ZTabWidget(QWidget *parent = nullptr);
    QTabBar *tabBar() const;
};

#endif // ZSCROLLAREA_H
