#ifndef ZMANGAVIEW_H
#define ZMANGAVIEW_H

#include <QtCore>
#include <QtGui>

typedef QList<QImage> QImageList;

class ZMangaView : public QGraphicsView
{
    Q_OBJECT
private:
    QImageList* iList;

public:
    int currentPage;
    explicit ZMangaView(QWidget *parent = 0);
    void setImageList(QImageList* ilist);
    void setPage(int page);
    
signals:
    void loadPage(int num);

public slots:
    
protected:
    void wheelEvent(QWheelEvent *event);
};

#endif // ZMANGAVIEW_H
