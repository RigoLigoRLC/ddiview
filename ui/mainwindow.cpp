#include <QFileDialog>
#include <QProgressDialog>
#include <QFile>
#include <QInputDialog>
#include <QTableWidget>
#include <QMessageBox>
#include "mainwindow.h"
#include "chunk/chunkcreator.h"
#include "parser/ddi.h"
#include "./ui_mainwindow.h"
#include "statisticsresultdialog.h"
#include "articulationtabledialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    SetupUI();

    mTreeRoot = nullptr;
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

    ui->grpDdb->setHidden(true);
    ui->grpMediaTool->setHidden(true);

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

BaseChunk *MainWindow::SearchForChunkByPath(QStringList paths)
{
    BaseChunk* baseSearchChunk = mTreeRoot;
    if(baseSearchChunk == nullptr) {
        QMessageBox::critical(this, tr("No root"), tr("Tree root chunk is non existent."));
        return nullptr;
    }
    foreach(auto i, paths) {
        if(i.isEmpty()) {
            QMessageBox::critical(this, tr("Invalid pattern"), tr("Empty name in path chain."));
            return nullptr;
        }
        baseSearchChunk = baseSearchChunk->GetChildByName(i);
        if(baseSearchChunk == nullptr) {
            QMessageBox::critical(this, tr("Specified chunk not found"), tr("Search for \"%1\" returned nothing.").arg(i));
            return nullptr;
        }
    }

    return baseSearchChunk;
}

void MainWindow::PatternedRecursionOnProperties(QString pattern, std::function<void (void *, BaseChunk *, QString)> iterfunc, void *iterCtx)
{
    // Input sanity checks
    // Only one dot allowed, escaping is not considered
    auto splitByDot = pattern.split('.');
    if(splitByDot.size() > 2) {
        QMessageBox::critical(this, tr("Invalid pattern"), tr("Too many \".\" operators."));
        return;
    }
    if(splitByDot.size() == 1) {
        QMessageBox::critical(this, tr("Invalid pattern"), tr("Did not specify property with \".\" operator."));
        return;
    }
    if(splitByDot[0].isEmpty()) {
        QMessageBox::critical(this, tr("Invalid pattern"), tr("Empty base path."));
        return;
    }
    if(splitByDot[1].isEmpty()) {
        QMessageBox::critical(this, tr("Invalid pattern"), tr("Empty property name."));
        return;
    }
    // Assume dot is right after wildcards, and only maximum of four asterisks are allowed
    int asteriskCount = 0;
    bool seekingForExtraAsterisks = false;
    for(auto i = splitByDot[0].constEnd(); ;) {
        if(*(--i) == '*') {
            if(!seekingForExtraAsterisks) asteriskCount++;
            else {
                QMessageBox::critical(this, tr("Invalid pattern"), tr("Extra \"*\" in the path."));
                return;
            }
        }
        else if(!seekingForExtraAsterisks) seekingForExtraAsterisks = true;
        if(asteriskCount >= 5) {
            QMessageBox::critical(this, tr("Invalid pattern"), tr("Too many \"*\" wildcards."));
            return;
        }
        if(i == splitByDot[0].cbegin()) break;
    }

    // Split paths, remove the leading slash first
    if(splitByDot[0][0] != '/') {
        QMessageBox::critical(this, tr("Invalid pattern"), tr("Path not beginning with \"/\"."));
        return;
    }
    auto paths = splitByDot[0].remove(0, 1).split('/');
    auto propName = splitByDot[1];
    const int maxDepth = paths.last().length(); // Number of '*'
    paths.removeLast();

    // Find the specified base search chunk
    BaseChunk* baseSearchChunk = SearchForChunkByPath(paths);
    if(baseSearchChunk == nullptr)
        return;

    // Decide recursion depth. Use iteration instead of recursion.
    QVector<QVector<BaseChunk*>*> childrenStack;
    QVector<int> childrenIndexStack, childrenMaxidxStack;
    childrenStack.resize(maxDepth);
    childrenIndexStack.resize(maxDepth);
    childrenMaxidxStack.resize(maxDepth);

    int depth = 0;

    // Initialize first layer
    childrenStack[0] = &baseSearchChunk->Children;
    childrenMaxidxStack[0] = baseSearchChunk->Children.count() - 1;
    childrenIndexStack[0] = 0;

    // Begin iterating
    forever {
        // Exit condition
        if(childrenIndexStack[depth] >= childrenMaxidxStack[depth]) {
            if(depth == 0) break;
            // Increment the current children index of last layer
            childrenIndexStack[depth - 1]++;
            depth--; // Exit one layer
            continue;
        }

        if(depth < maxDepth - 1) {
            // Have not touched bottom yet
            // Prepare next layer at current chilren index
            childrenStack[depth + 1] = &childrenStack[depth]->at(childrenIndexStack[depth])->Children;
            childrenMaxidxStack[depth + 1] = childrenStack[depth + 1]->count() - 1;
            childrenIndexStack[depth + 1] = 0;

            // Enter next layer of iteration
            depth++; continue;
        } else {
            // Already touched the bottom, do iterations
            foreach(auto i, *childrenStack[depth]) {
                iterfunc(iterCtx, i, propName);

                childrenIndexStack[depth]++;
            }
        }
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

    mTreeRoot = root;
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
    std::string x;
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
                                       "/voice/stationary/normal/*.Index (The Index property of each item under /voice/stationary)"),
                                    QLineEdit::Normal,
                                    QString(),
                                    &ok);

    if(!ok) return;

    struct PropDistIterCtx {
        QMap<QByteArray, uint32_t> statistics;
        uint32_t nonExistentCount;
    } ctx { {}, 0 };

    auto iterfunc = [](void *ctx, BaseChunk *chunk, QString propName){
        PropDistIterCtx *_ctx = reinterpret_cast<PropDistIterCtx*>(ctx);

        auto propMap = chunk->GetPropertiesMap();
        if(propMap.contains(propName)) {
            _ctx->statistics[propMap[propName]]++;
        } else {
            _ctx->nonExistentCount++;
        }
    };

    PatternedRecursionOnProperties(pattern, iterfunc, &ctx);

    StatisticsResultDialog resultDlg;
    resultDlg.SetResult(ctx.statistics, ctx.nonExistentCount);
    resultDlg.setWindowTitle(tr("Result of \"%1\"").arg(pattern));
    resultDlg.exec();
}


void MainWindow::on_btnShowDdb_toggled(bool checked)
{
    ui->grpDdb->setVisible(checked);
}


void MainWindow::on_btnShowMediaTool_toggled(bool checked)
{
    ui->grpMediaTool->setVisible(checked);
}


void MainWindow::on_actionArticulationTable_triggered()
{
    QStringList phonemeList;
    QVector<int> supportMatrix;

    BaseChunk *phonemesChunk = SearchForChunkByPath({ "<Phoneme Dictionary>", "<Phonemes>" }),
            *articulationChunk = SearchForChunkByPath({ "voice", "articulation" });
    if(phonemesChunk == nullptr || articulationChunk == nullptr)
        return;



    // Obtain all phonemes
    foreach(const auto &i, phonemesChunk->Children) {
        phonemeList << i->GetName();
    }

    supportMatrix.resize(phonemeList.size() * phonemeList.size());
    supportMatrix.fill(-1);

    foreach(const auto &i, articulationChunk->Children) {
        // No articulation beginning with this
        if(i->Children.size() == 0) {
            auto begin = phonemeList.indexOf(i->GetName()) * phonemeList.size();
            // Fill entire row with -2
            for(auto j = 0; j < phonemeList.size(); j++)
                supportMatrix[begin + j] = -2;
            continue;
        }
        foreach(const auto &j, i->Children) {
            if(j->GetSignature() == "ART ") {
                // TODO: Triphoneme
                continue;
            }
            auto data = j->GetProperty("Count").data;
            int count;
            STUFF_INTO(data, count, 4);
            supportMatrix[phonemeList.indexOf(i->GetName()) * phonemeList.size() +
                    phonemeList.indexOf(j->GetName())] = count;
        }
    }

    ArticulationTableDialog dlg;
    dlg.setWindowTitle("Articulation Table - " + mLblStatusFilename->text());
    dlg.SetSupportMatrix(phonemeList, supportMatrix);
    dlg.exec();
}

