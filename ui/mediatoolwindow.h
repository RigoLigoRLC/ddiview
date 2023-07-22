#ifndef MEDIATOOLWINDOW_H
#define MEDIATOOLWINDOW_H

#include <QMainWindow>

namespace Ui {
class MediaToolWindow;
}

class MediaToolWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MediaToolWindow(QWidget *parent = nullptr);
    ~MediaToolWindow();

private:
    Ui::MediaToolWindow *ui;
};

#endif // MEDIATOOLWINDOW_H
