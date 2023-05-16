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

    BaseChunk *SearchForChunkByPath(QStringList paths);

    void PatternedRecursionOnProperties(QString pattern, std::function<void (void *, BaseChunk *, QString)> iterfunc, void* iterCtx);

    bool EnsureDdbExists();

private slots:
    void on_actionExit_triggered();

    void on_actionOpen_triggered();

    void on_treeStructure_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

    void on_listProperties_customContextMenuRequested(QPoint point);

    void on_actionPropDist_triggered();

    void on_btnShowDdb_toggled(bool checked);

    void on_btnShowMediaTool_toggled(bool checked);

    void on_actionArticulationTable_triggered();

    void on_actionExportJson_triggered();

    void on_actionExtractAllSamples_triggered();

    void on_actionactionExportDdbLayout_triggered();

private:
    Ui::MainWindow *ui;
    QLabel *mLblStatusFilename,
           *mLblStatusDdbExists,
           *mLblBlockOffset;

    BaseChunk* mTreeRoot;

private:
    QString mDatabaseDirectory;
    bool mDdbLoaded, mDdbSelectionDirty;

};
#endif // MAINWINDOW_H
