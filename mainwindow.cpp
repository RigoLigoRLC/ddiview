#include <QFileDialog>
#include <QProgressDialog>
#include <QFile>
#include <QInputDialog>
#include "mainwindow.h"
#include "chunk/chunkcreator.h"
#include "parser/ddi.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    SetupUI();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::SetupUI()
{
    mLblStatusFilename = new QLabel();
    mLblBlockOffset = new QLabel();
    ui->statusbar->addWidget(mLblStatusFilename, 999);
    ui->statusbar->addWidget(mLblBlockOffset);

    resize(1000, 800);
}

void MainWindow::BuildTree(BaseChunk *chunk, QTreeWidgetItem *into)
{
    auto t = ui->treeStructure;
    QTreeWidgetItem *rootItem;

    if(into == nullptr) {
        t->clear();
        rootItem = new QTreeWidgetItem(t);
    } else {
        rootItem = new QTreeWidgetItem(into);
    }

    rootItem->setText(0, chunk->GetName());
    rootItem->setText(1, chunk->Description());
    rootItem->setData(0, BaseChunk::ItemChunkRole, QVariant::fromValue<BaseChunk*>(chunk));

    foreach(auto i, chunk->Children) {
        BuildTree(i, rootItem);
    }
}

void MainWindow::on_actionExit_triggered()
{
    close();
}


void MainWindow::on_actionOpen_triggered()
{
    QFileDialog file;
    QString filename;

    filename = file.getOpenFileName(
            this,
            tr("Open DDI..."),
            QDir::currentPath(),
            "Daisy Database Index (*.ddi)");

    if(filename.isEmpty())
        return;

    if(ui->treeStructure->topLevelItemCount())
        delete ui->treeStructure->topLevelItem(0)->data(0, BaseChunk::ItemChunkRole).value<BaseChunk*>();

    // FIXME: If the signal is connected when doing clear, the clear method will deadlock.
    disconnect(ui->treeStructure, &QTreeWidget::currentItemChanged, this, &MainWindow::on_treeStructure_currentItemChanged);
    ui->treeStructure->clear();
    connect(ui->treeStructure, &QTreeWidget::currentItemChanged, this, &MainWindow::on_treeStructure_currentItemChanged);

    QString fileBasename = filename.section('/', -1);
    QFile fileForSize(filename);
    QProgressDialog progDlg(QString("Reading %1...").arg(fileBasename),
                            QString(),
                            0,
                            fileForSize.size(),
                            this);
    progDlg.setWindowModality(Qt::WindowModal);
    progDlg.setMinimumDuration(0);
    mLblStatusFilename->setText(fileBasename);

    // TODO: use factory method
    ChunkCreator::Get()->SetProgressDialog(&progDlg);
    auto root = Parse(filename);
    ChunkCreator::Get()->SetProgressDialog(nullptr);

    BuildTree(root, nullptr);
}


void MainWindow::on_treeStructure_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    if(!current)
        return;

    auto chunk = current->data(0, BaseChunk::ItemChunkRole).value<BaseChunk*>();
    auto props = chunk->GetPropertiesMap();

    ui->listProperties->clear();
    for(auto i = props.cbegin(); i != props.cend(); i++) {
        QString propText;
        propText += i.key() + tr(" (%1 bytes)\n").arg(i.value().data.size())
                  + FormatProperty(i.value());
        auto item = new QListWidgetItem(propText);

        // Make those "known values" (with proper typing) a bit more eye catching
        if(i.value().type != PropRawHex)
            item->setBackground(QColor(200, 255, 200));
        ui->listProperties->addItem(item);

    }

    mLblBlockOffset->setText(QString::number(chunk->GetOriginalOffset(), 16).toUpper());
}


void MainWindow::on_actionPropDist_triggered()
{
    bool ok;
    QString pattern;
    pattern = QInputDialog::getText(this,
                                    tr("Property Distribution"),
                                    tr("Enter pattern.\n"
                                       "Examples:\n\n"
                                       "/voice/articulation/**.unk12 (The unk12 property of every child under each chunk in /voice/articulation)\n"
                                       "/voice/stationary/normal/*.Index (The Index property of each item under /voice/stationary)"));


}

