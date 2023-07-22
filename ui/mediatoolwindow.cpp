#include "mediatoolwindow.h"
#include "ui_mediatoolwindow.h"

MediaToolWindow::MediaToolWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MediaToolWindow)
{
    ui->setupUi(this);
}

MediaToolWindow::~MediaToolWindow()
{
    delete ui;
}
