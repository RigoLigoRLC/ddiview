#include "ui/mainwindow.h"

#include <QApplication>
#include "chunk/basechunk.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.show();
    return a.exec();
}
