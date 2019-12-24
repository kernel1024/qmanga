#include <QApplication>
#include "mainwindow.h"
#include "global.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    zF->initialize();
    ZMainWindow w;
    w.show();
    return a.exec();
}
