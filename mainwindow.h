#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QLabel>
#include "chunk/basechunk.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void SetupUI();

    void BuildTree(BaseChunk* chunk, QTreeWidgetItem* into);

private slots:
    void on_actionExit_triggered();

    void on_actionOpen_triggered();

    void on_treeStructure_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

    void on_actionPropDist_triggered();

private:
    Ui::MainWindow *ui;
    QLabel *mLblStatusFilename, *mLblBlockOffset;

    BaseChunk* mTreeRoot;
};
#endif // MAINWINDOW_H
