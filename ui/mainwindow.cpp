
#define _CRT_SECURE_NO_WARNINGS

#include <QFileDialog>
#include <QProgressDialog>
#include <QFile>
#include <QInputDialog>
#include <QTableWidget>
#include <QMessageBox>
#include "chunk/chunkreaderguards.h"
#include "mainwindow.h"
#include "chunk/chunkcreator.h"
#include "chunk/soundchunk.h"
#include "chunk/dbvstationaryphupart_devdb.h"
#include "chunk/dbvarticulationphu_devdb.h"
#include "chunk/dbvarticulationphupart_devdb.h"
#include "parser/ddi.h"
#include "./ui_mainwindow.h"
#include "qdebug.h"
#include "statisticsresultdialog.h"
#include "articulationtabledialog.h"
#include "ddiexportjsonoptionsdialog.h"
#include "propertycontextmenu.h"
#include "common.h"
#include "util/util.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow),
    mDdbStream(&mDdbFile)
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
    mLblStatusDdbExists = new QLabel();
    mLblBlockOffset = new QLabel();
    mLblPropertyOffset = new QLabel();
    ui->statusbar->addWidget(mLblStatusFilename);
    ui->statusbar->addWidget(mLblStatusDdbExists, 999);
    ui->statusbar->addWidget(mLblBlockOffset);
    ui->statusbar->addWidget(mLblPropertyOffset);

    ui->grpDdb->setHidden(true);
    //ui->grpMediaTool->setHidden(true);

    mDdbLoaded = false;
    mDdbSelectionDirty = false;

    auto pageWvfmLay = new QVBoxLayout;
    ui->pageWaveform->setLayout(pageWvfmLay);
    pageWvfmLay->addWidget((mWaveformPlot = new QCustomPlot));
    mWaveformPlot->yAxis->setRange(-1.0, 1.0);
    mWaveformGraph = new QCPGraph(mWaveformPlot->xAxis, mWaveformPlot->yAxis);

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

void MainWindow::BuildDdb(QProgressDialog *dlg)
{
    FILE* f = fopen(mDdbPath.toLocal8Bit(), "rb");
    if (!f) {
        return;
    }

    dlg->reset();
    dlg->setLabelText(tr("Reading DDB chunks..."));

    myfseek64(f, 0, SEEK_END);
    auto length = myftell64(f);
    dlg->setMaximum(length >> 4); // Qt uses 32 bit integer only, we must discard a few bits
    myfseek64(f, 0, SEEK_SET);

    auto rootItem = ui->treeStructureDdb;
    uint64_t lastOffset = -1;

    BaseChunk::HasLeadingQword = false;
    while (true) {
        char sig[5] = {0, 0, 0, 0, 0};
        assert(myftell64(f) > lastOffset || lastOffset == -1);

        if (myftell64(f) >= length) {
            break;
        }

        lastOffset = myftell64(f);
        fread(sig, 1, 4, f);
        myfseek64(f, myftell64(f) - 4, SEEK_SET);
        dlg->setValue(myftell64(f) >> 4);

        if (!strncmp(sig, "SND ", 4) || !strncmp(sig, "FRM2", 4)) {
            auto chk = ChunkCreator::Get()->ReadFor(QByteArray(sig, 4), f);
            // We DELIBERATELY use END offset so that lower_bound will happily return
            // the first chunk tail that we will meet after the requested point
            mDdbChunks[chk->GetOriginalOffset() + chk->GetSize() - 1] = chk;
        } else {
            // Skip chunk
            auto chk = ChunkCreator::Get()->ReadFor("____Skipped", f);
            mDdbChunks[chk->GetOriginalOffset() + chk->GetSize() - 1] = chk;
        }
    }

    // DDI -> DDB Linkage
    auto procPitch = [=](BaseChunk* pitch, QTreeWidgetItem* treeParent){
        auto phPitch = new QTreeWidgetItem(treeParent, {pitch->GetName()});

        auto phSnd = new QTreeWidgetItem(phPitch, {tr("Sound")});
        auto sndOffsetProp = pitch->GetProperty("SND Sample offset");
        if (sndOffsetProp.type == PropHex64) {
            uint64_t offset;
            STUFF_INTO(sndOffsetProp.data, offset, uint64_t);
            auto foundChunk = mDdbChunks.lower_bound(offset);
            if (foundChunk->second->ObjectSignature() == "SND ") {
                phSnd->setText(1, QString::number(offset, 16));
                phSnd->setData(0, BaseChunk::ItemChunkRole, QVariant::fromValue<BaseChunk*>(foundChunk->second));
                mDdbChunks.erase(foundChunk);
            } else {
                qDebug() << "SND Look for:" << QString::number(offset, 16) << "Found:"
                         << QString::number(foundChunk->second->GetOriginalOffset(), 16)
                         << "Signature" << QString(foundChunk->second->GetSignature());
            }
        }

        auto frames = pitch->GetChildByName("<Frames>");
        if (!frames) return;

#if 0
        auto &&propMap = frames->GetPropertiesMap();
        for (auto it = propMap.begin(); it != propMap.end(); it++) {
            if (it.key() == "Count") continue;
            if (it->type == PropHex64) {
                auto frame = new QTreeWidgetItem(phPitch);
                uint64_t offset;
                STUFF_INTO(it->data, offset, uint64_t);
                auto foundChunk = mDdbChunks.lower_bound(offset);
                if (foundChunk->second->GetOriginalOffset() == offset && foundChunk->second->ObjectSignature() == "FRM2") {
                    // Must exactly match
                    frame->setText(0, it.key());
                    frame->setText(1, QString::number(offset, 16));
                    frame->setData(0, BaseChunk::ItemChunkRole, QVariant::fromValue<BaseChunk*>(foundChunk->second));
                    mDdbChunks.erase(foundChunk);
                } else {
                    qDebug() << "FRM2 Look for:" << QString::number(offset, 16) << "Found:"
                             << QString::number(foundChunk->second->GetOriginalOffset(), 16)
                             << "Signature" << QString(foundChunk->second->GetSignature());
                }
            }
        }
#endif
    };

    auto stationaryDdbRoot = new QTreeWidgetItem(rootItem, {"stationary"});
    auto stationaryRoot = SearchForChunkByPath({"voice", "stationary", "normal"});
    if (stationaryRoot) {
        foreach (auto const ph, stationaryRoot->Children) {
            auto phDdb = new QTreeWidgetItem(stationaryDdbRoot, {ph->GetName()});
            foreach (auto const pitch, ph->Children) {
                procPitch(pitch, phDdb);
            }
        }
    }

    auto articulationDdbRoot = new QTreeWidgetItem(rootItem, {"articulation"});
    auto articulationRoot = SearchForChunkByPath({"voice", "articulation"});
    if (articulationRoot) {
        foreach (auto const ph1, articulationRoot->Children) {
            auto ph1Ddb = new QTreeWidgetItem(articulationDdbRoot, {ph1->GetName()});
            foreach (auto const ph2, ph1->Children) {
                auto ph2Ddb = new QTreeWidgetItem(ph1Ddb, {ph2->GetName()});

                if (ph2->GetSignature() == "ART ") {
                    // Triphoneme
                    foreach (auto const ph3, ph2->Children) {
                        auto ph3Ddb = new QTreeWidgetItem(ph2Ddb, {ph3->GetName()});
                        foreach (auto const pitch, ph3->Children) {
                            procPitch(pitch, ph3Ddb);
                        }
                    }
                } else {
                    // Diphoneme
                    foreach (auto const pitch, ph2->Children) {
                        procPitch(pitch, ph2Ddb);
                    }
                }
            }
        }
    }

    // Orphan chunks
#if 0
    for (auto it = mDdbChunks.begin(); it != mDdbChunks.end(); it++) {
        auto chunkDdb = new QTreeWidgetItem(rootItem, {tr("Orphan chunk <%1>").arg(QString(it->second->GetSignature()))});
        chunkDdb->setText(1, QString::number(it->second->GetOriginalOffset(), 16));
        chunkDdb->setData(0, BaseChunk::ItemChunkRole, QVariant::fromValue<BaseChunk*>(it->second));
    }
#endif
    mDdbChunks.clear();

    fclose(f);

    mDdbFile.close();
    mDdbFile.setFileName(mDdbPath);
    mDdbFile.open(QFile::OpenModeFlag::ReadOnly);
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

bool MainWindow::EnsureDdbExists()
{
    bool ret;
    auto ddbPath = mDatabaseDirectory + '/' + mLblStatusFilename->text().section('.', 0, -2) + ".ddb";
    ret = QFile::exists(ddbPath);
    auto font = mLblStatusDdbExists->font();
    if(ret) {
        font.setBold(false);
        mLblStatusDdbExists->setText(tr("DDB Exists"));
        mLblStatusDdbExists->setStyleSheet("color: green; font-weight: 500;");
        mDdbPath = ddbPath;
    } else {
        font.setBold(true);
        mLblStatusDdbExists->setText(tr("DDB Missing"));
        mLblStatusDdbExists->setStyleSheet("color: red; font-weight: 700;");
        mDdbPath.clear();
    }
    return ret;
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
            "Daisy Database Index (*.ddi);;Development VB Index (*.tree)");

    if(filename.isEmpty())
        return;

    if(ui->treeStructure->topLevelItemCount())
        delete ui->treeStructure->topLevelItem(0)->data(0, BaseChunk::ItemChunkRole).value<BaseChunk*>();

    // FIXME: If the signal is connected when doing clear, the clear method will deadlock.
    disconnect(ui->treeStructure, &QTreeWidget::currentItemChanged, this, &MainWindow::on_treeStructure_currentItemChanged);
    ui->treeStructure->clear();
    connect(ui->treeStructure, &QTreeWidget::currentItemChanged, this, &MainWindow::on_treeStructure_currentItemChanged);

    mDdiPath = filename;
    QString fileBasename = filename.section('/', -1);
    QFile fileForSize(filename);
    QProgressDialog progDlg(QString("Reading %1...").arg(fileBasename),
                            QString(),
                            0,
                            fileForSize.size(),
                            this);
    progDlg.setWindowModality(Qt::WindowModal);
    progDlg.setMinimumDuration(0);
    progDlg.setAutoClose(false);
    mLblStatusFilename->setText(fileBasename);
    mDatabaseDirectory = filename.section('/', 0, -2);

    // TODO: use factory method
    ChunkCreator::Get()->SetProgressDialog(&progDlg);
    auto root = ParseDdi(filename);
    ChunkCreator::Get()->SetProgressDialog(nullptr);

    mTreeRoot = root;
    BuildTree(root, nullptr);

    if (EnsureDdbExists()) {
        // DDB read. DDB is not cached to RAM because it is very large, data is read on demand
        BuildDdb(&progDlg);
    }

    progDlg.close();
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
        item->setData(BaseChunk::ItemPropDataRole, i.value().data);
        item->setData(BaseChunk::ItemOffsetRole, i.value().offset);

        // Make those "known values" (with proper typing) a bit more eye catching
        if(i.value().type != PropRawHex)
            item->setBackground(QColor(200, 255, 200));
        ui->listProperties->addItem(item);

    }

    mLblBlockOffset->setText("NODE " + QString::number(chunk->GetOriginalOffset(), 16).toUpper());
    std::string x;
}

void MainWindow::on_listProperties_customContextMenuRequested(QPoint point)
{
    auto item = ui->listProperties->itemAt(point);
    if(!item) return;

    PropertyContextMenu menu(this, item);
    auto globPoint = ui->listProperties->mapToGlobal(point);
    menu.exec(globPoint);
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
    //ui->grpMediaTool->setVisible(checked);
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
                continue;
                foreach(auto thirdPhoneme, j->Children) {
                    // TODO: Triphoneme
                }
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
    dlg.SetDiphonemeMatrix(phonemeList, supportMatrix);
    dlg.exec();
}


void MainWindow::on_actionExportJson_triggered()
{
    auto rootChunk = SearchForChunkByPath({ });
    if(!rootChunk) return;

    QFileDialog filedlg;
    QString filename;

    filename = filedlg.getSaveFileName(
            this,
            tr("Save JSON..."),
            QDir::currentPath(),
            "JSON file (*.json)");
    if(filename.isEmpty())
        return;

    QFile file(filename);
    file.open(QFile::WriteOnly);
    if(!file.isWritable()) {
        QMessageBox::critical(this, tr("Failed to export articulations"), tr("File is not writable"));
        return;
    }
    QTextStream stream(&file);

    DdiExportJsonOptionsDialog cfgDlg;
    if(cfgDlg.exec() == QDialog::DialogCode::Rejected)
        return;

    auto sanitizeString = [](QString s) -> QString {
        QString ret; QChar prev = 0;
        for(auto i : s) {
            if((i == '\"' || i == '\\') && (prev != '\\')) {
                ret += '\\'; ret += i;
            } else {
                ret += i;
            }
            prev = i;
        }
        return ret;
    };

    int indentLevel = 0;
    bool doCompact = cfgDlg.GetDoUseCompactJson(),
         doVerbatimPropValue = cfgDlg.GetDoUseVerbatimPropValues();

    auto outputIndent = [=](QTextStream* _stream, int level){
        if(doCompact) return;
        *_stream << '\n';
        for(int i = 0; i < level; i++) *_stream << "    ";
    };

    std::function<void(QTextStream*,BaseChunk*)> outputChunkAndChildren = [&](QTextStream* stream, BaseChunk* chunk){
        // Basic information
        *stream << "{";
        indentLevel++;
        outputIndent(stream, indentLevel);
        *stream << "\"name\":\"" << sanitizeString(chunk->GetName()) << "\",";
        outputIndent(stream, indentLevel);
        *stream << "\"signature\":\"" << chunk->ObjectSignature() << "\"";
        // Properties
        auto propMap = chunk->GetPropertiesMap();
        if(!propMap.isEmpty()) {
            *stream << ",";
            outputIndent(stream, indentLevel);
            *stream << "\"properties\":{";
            indentLevel++;
            for(auto i = propMap.begin(); i != propMap.end(); ) {
                outputIndent(stream, indentLevel);
                if(doVerbatimPropValue) {
                    *stream << '\"' << i.key()
                            << "\":[" << i.value().type << ",\""
                            << i.value().data.toHex() << "\"]";
                } else {
                    *stream << '\"' << i.key()
                            <<"\":\""
                            << FormatProperty(i.value()) << "\"";
                }
                if(++i != propMap.end()) {
                    *stream << ',';
                } else {
                    indentLevel--;
                    outputIndent(stream, indentLevel);
                    *stream << '}';
                }
            }
        }

        // Children
        if(!chunk->Children.isEmpty()) {
            auto children = chunk->Children;
            *stream << ",";
            outputIndent(stream, indentLevel);
            *stream << "\"children\":[";
            indentLevel++;
            for(auto i = children.cbegin(); i != children.cend(); ) {
                outputIndent(stream, indentLevel);
                outputChunkAndChildren(stream, *i);
                if(++i != children.cend())
                    *stream << ',';
            }
            indentLevel--;
            outputIndent(stream, indentLevel);
            *stream << ']';
        }
        indentLevel--;
        outputIndent(stream, indentLevel);
        *stream << '}';
    };

    outputChunkAndChildren(&stream, rootChunk);
}


void MainWindow::on_actionExtractAllSamples_triggered()
{
    if(!EnsureDdbExists()) {
        QMessageBox::critical(this,
                              tr("No DDB found"),
                              tr("Cannot extract samples. Please ensure DDB is in place."));
        return;
    }

    QFile ddbFile(mDatabaseDirectory + '/' + mLblStatusFilename->text().section('.', 0, -2) + ".ddb");
    if(!ddbFile.open(QFile::ReadOnly)) {
        QMessageBox::critical(this,
                              tr("Cannot open DDB!"),
                              tr("Can't open \"%1\" for read.").arg(ddbFile.fileName()));
        return;
    }

    QString dir, fullSubdir;
    QDir qd;

    bool dirSelectNotOk = true, doNotCreateSubdir = false;
    do {
        dir = QFileDialog::getExistingDirectory(this,
                                                tr("Choose destination (will extract to subdirectory)"),
                                                qApp->applicationDirPath(),
                                                QFileDialog::ShowDirsOnly
                                                | QFileDialog::DontResolveSymlinks);

        if(dir.isEmpty())
            return;

        fullSubdir = dir + "/" + mLblStatusFilename->text();
        if(qd.exists(fullSubdir)) {
            auto result = QMessageBox::warning(this,
                                               tr("Destination exists"),
                                               tr("\"%1\" already exists in the destination.\n"
                                                  "Do you wish to choose another directory?").arg(fullSubdir),
                                               QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            switch(result) {
            case QMessageBox::Yes: break;
            case QMessageBox::No: dirSelectNotOk = false; doNotCreateSubdir = true; break;
            default: return;
            }
        } else {
            dirSelectNotOk = false;
        }
    } while (dirSelectNotOk);

    if(!doNotCreateSubdir && !qd.mkdir(fullSubdir)) {
        QMessageBox::critical(this,
                              tr("Cannot create subdirectory!"),
                              tr("Cannot create subdirectory for \"%1\".").arg(mLblStatusFilename->text()));
        return;
    }

    // Build task list
    struct Section {
        int sectionLB, sectionUB,
            stationarySectionLB, stationarySectionUB;
    };

    struct ExtractTask {
        enum { Stationary, Articulation, Triphoneme } type;
        uint64_t ddbOffset;
        uint32_t extractBytes;
        int midiPitch; // A4 = 69
        QString voiceColor; // stationary/normal, articulation/xx/xx/default
        QString name;
        QVector<Section> sections;
        QVector<QString> phonemes;
        uint32_t totalFrames;
    };

    QVector<ExtractTask> taskList;
    QProgressDialog progDlg(QString(),
                            QString(),
                            0,
                            1,
                            this);
    progDlg.setWindowModality(Qt::WindowModal);
    progDlg.setMinimumDuration(0);
    progDlg.setAutoReset(false);
    progDlg.setMaximum(0);
    progDlg.setValue(0);
    progDlg.setLabelText(tr("Building extraction task list..."));
    progDlg.show();

    // Iterate stationaries
    QVector<QString> stationaryColors;
    auto stationaryRoot = SearchForChunkByPath({ "voice", "stationary" });
    do {
        if(!stationaryRoot) break;
        // Iterate voice colors
        foreach(auto voiceColor, stationaryRoot->Children) {
            stationaryColors << voiceColor->GetName();
            // Iterate stationary segments
            foreach(auto staSeg, voiceColor->Children) {
                // Iterate each pitch of the segment
                foreach(auto pitchSeg, staSeg->Children) {
                    ExtractTask task;
                    float relativePitch = 0.0f;
                    task.voiceColor = voiceColor->GetName();
                    task.type = ExtractTask::Stationary;
                    STUFF_INTO(pitchSeg->GetProperty("SND Sample offset").data, task.ddbOffset, uint64_t);
                    STUFF_INTO(pitchSeg->GetProperty("mPitch").data, relativePitch, float);
                    STUFF_INTO(pitchSeg->GetProperty("SND Sample count").data, task.extractBytes, uint32_t);
                    STUFF_INTO(pitchSeg->GetProperty("Frame count").data, task.totalFrames, uint32_t);
                    task.extractBytes -= 0x800;
                    task.extractBytes *= sizeof(uint16_t);
                    task.midiPitch = Common::RelativePitchToMidiNote(relativePitch);
                    task.name = staSeg->GetName();
                    task.phonemes << task.name;
                    taskList << task;

                    progDlg.setValue(1);
                }
            }
        }
    } while(0);

    // Iterate articulations
    QVector<QString> articulationColors;
    auto articulationRoot = SearchForChunkByPath({ "voice", "articulation" });
    do {
        if(!articulationRoot) break;
        // Iterate beginPhonemes
        foreach(auto beginPhoneme, articulationRoot->Children) {
            // Iterate end phonemes
            foreach(auto endPhoneme, beginPhoneme->Children) {
                // TODO: Triphonemes skipped
                if(endPhoneme->ObjectSignature() == "ART ") {
                    continue;
                    foreach(auto thirdPhoneme, endPhoneme->Children) {
                        foreach(auto pitchSeg, thirdPhoneme->Children) {

                        }
                    }
                }

                // Iterate each pitch of the segment
                foreach(auto pitchSeg, endPhoneme->Children) {
                    if(!articulationColors.contains(pitchSeg->GetName()))
                        articulationColors << pitchSeg->GetName();

                    ExtractTask task;
                    float relativePitch = 0.0f;
                    task.voiceColor = pitchSeg->GetName();
                    task.type = ExtractTask::Articulation;
                    STUFF_INTO(pitchSeg->GetProperty("SND Sample offset").data, task.ddbOffset, uint64_t);
                    STUFF_INTO(pitchSeg->GetProperty("mPitch").data, relativePitch, float);
                    STUFF_INTO(pitchSeg->GetProperty("SND Sample count").data, task.extractBytes, uint32_t);
                    STUFF_INTO(pitchSeg->GetProperty("Frame count").data, task.totalFrames, uint32_t);
                    task.extractBytes -= 0x800;
                    task.extractBytes *= sizeof(uint16_t);
                    task.midiPitch = Common::RelativePitchToMidiNote(relativePitch);
                    task.name = beginPhoneme->GetName() + "[To]" + endPhoneme->GetName();
                    task.phonemes << beginPhoneme->GetName() << endPhoneme->GetName();

                    // Sections
                    auto sectionsDir = pitchSeg->GetChildByName("<sections>");
                    do {
                        if(!sectionsDir) break;

                        foreach(auto sec, sectionsDir->Children) {
                            Section section;
                            STUFF_INTO(sec->GetProperty("Entire section Begin").data, section.sectionLB, uint32_t);
                            STUFF_INTO(sec->GetProperty("Entire section End").data, section.sectionUB, uint32_t);
                            STUFF_INTO(sec->GetProperty("Stationary section Begin").data, section.stationarySectionLB, uint32_t);
                            STUFF_INTO(sec->GetProperty("Stationary section End").data, section.stationarySectionUB, uint32_t);
                            task.sections << section;
                        }
                    } while(0);

                    taskList << task;
                    progDlg.setValue(1);
                }
            }
        }
    } while(0);

    // TODO: Triphonemes

    progDlg.reset();

    // Start extracting
    // We don't care about error reporting now
    QFile csvSections(fullSubdir + '/' + "SECTIONS.CSV");
    csvSections.open(QFile::WriteOnly);
    QFile otoFile(fullSubdir + '/' + "oto.ini");
    otoFile.open(QFile::WriteOnly);

    QTextStream csvStream(&csvSections);
    csvStream << "Type,Filename,Color,Phonemes,Alias,Pitch,Length,SectionBegin,SectionEnd,StationaryBegin,StationaryEnd,\n";
    QTextStream oto(&otoFile);

    // Prepare sub sub directories
    foreach(auto i, stationaryColors)
        qd.mkdir(fullSubdir + "/stationary_" + i);
    foreach(auto i, articulationColors)
        qd.mkdir(fullSubdir + "/articulation_" + i);

    progDlg.setMaximum(taskList.size());
    progDlg.setValue(0);
    progDlg.setCancelButtonText(tr("Stop"));
    progDlg.show();

    for(auto i = 0; i < taskList.size(); i++) {
        if(progDlg.wasCanceled()) {
            QMessageBox::information(this,
                                     tr("Extraction cancelled"),
                                     tr("You've cancelled extraction task.\n"
                                        "%1/%2 tasks done.").arg(i).arg(taskList.size()));
            return;
        }

        static uint8_t copyBuffer[4096];
        QString path = fullSubdir;
        ExtractTask &task = taskList[i];
        switch(task.type) {
        case ExtractTask::Stationary: path += "/stationary_" + task.voiceColor; break;
        case ExtractTask::Articulation: path += "/articulation_" + task.voiceColor; break;
        case ExtractTask::Triphoneme: path += "/triphoneme_" + task.voiceColor; break;
        }
        path += '/'
              + QString("%1").arg(i, 6, 10, QChar('0'))
              + '_'
              + Common::SanitizeFilename(task.name)
              + "_@" + Common::MidiNoteToNoteName(task.midiPitch)
              + ".wav";

// Sometimes I want only CSV to be exported so I'll disable this section and recompile
#if 1
        QFile dstWave(path);
        if(!dstWave.open(QFile::WriteOnly))
            continue;

        uint32_t tmp = 0;
        uint16_t tmp2 = 0;
        dstWave.write("RIFF", 4);
        tmp = 44 + task.extractBytes; dstWave.write((char*)&tmp, sizeof(tmp));
        dstWave.write("WAVE", 4);

        // Assume 44100Hz 16Bit PCM 1 channel
        dstWave.write("fmt ", 4);
        tmp = 16; dstWave.write((char*)&tmp, sizeof(tmp)); // Subchunk1 size
        tmp2 = 1; dstWave.write((char*)&tmp2, sizeof(tmp2)); // LPCM = 1
        tmp2 = 1; dstWave.write((char*)&tmp2, sizeof(tmp2)); // Channel
        tmp = 44100; dstWave.write((char*)&tmp, sizeof(tmp)); // Sample rate
        tmp = 44100 * 1 * 16 / 8; dstWave.write((char*)&tmp, sizeof(tmp)); // Byte rate
        tmp2 = 1 * 16 / 8; dstWave.write((char*)&tmp2, sizeof(tmp2)); // Block align
        tmp2 = 16; dstWave.write((char*)&tmp2, sizeof(tmp2)); // Bits per sample

        dstWave.write("data", 4);
        tmp = task.extractBytes; dstWave.write((char*)&tmp, sizeof(tmp));

        // Read-write loop
        ddbFile.seek(task.ddbOffset);
        for(auto x = task.extractBytes; x > 0; ) {
            int readAmount = (x > sizeof(copyBuffer)) ? sizeof(copyBuffer) : x;
            ddbFile.read((char*)copyBuffer, readAmount);
            dstWave.write((char*)copyBuffer, readAmount);
            x -= readAmount;
        }

        dstWave.close();
#endif

        // Write SECTIONS.CSV
        // We shall convert frame count to seconds
        double wavLength = task.extractBytes / sizeof(uint16_t) / 44100.0,
            dTotalFrames = double(task.totalFrames);

        QString phonemesStr;
        foreach(auto j, task.phonemes)
            phonemesStr += j + ' ';
        phonemesStr = phonemesStr.trimmed(); // trim the space at the end

        csvStream << task.type << ','
                  << path.section('/', -1, -1) << ','
                  << task.voiceColor << ','
                  << Common::EscapeStringForCsv(phonemesStr) << ','
                  << Common::EscapeStringForCsv(phonemesStr) << '_' << Common::MidiNoteToNoteName(task.midiPitch) << ','
                  << task.midiPitch << ','
                  << 1000.0 * wavLength << ',';
        foreach(auto &i, task.sections) {
            csvStream << 1000.0 * i.sectionLB / dTotalFrames * wavLength << ','
                      << 1000.0 * i.sectionUB / dTotalFrames * wavLength << ','
                      << 1000.0 * i.stationarySectionLB / dTotalFrames * wavLength << ','
                      << 1000.0 * i.stationarySectionUB / dTotalFrames * wavLength << ',';
        }
        csvStream << '\n';

        // output oto
        oto << path.section('/', -1, -1) << '=' << phonemesStr << '_' << Common::MidiNoteToNoteName(task.midiPitch) << ',';
        switch(task.type) {
        case ExtractTask::Stationary:
            oto << "0,0,0,0,120\n"; break; // Needs verify
        case ExtractTask::Articulation:
            oto << "0," // Begin
//                << 1000.0 * wavLength // Consonant(NoStretch)
                << "0," // Let it be completely stretchable for now...
                << ",0," // End
                << 1000.0 * task.sections[0].sectionUB / dTotalFrames * wavLength << ',' // Preutterance
                << 1000.0 * (task.sections[0].stationarySectionLB - task.sections[0].sectionLB)  / dTotalFrames * wavLength // Leading Overlap
                << '\n';
            break;
        default:
            break;
        }

        progDlg.setValue(i);

        if(i % 15 == 0) {
            progDlg.setLabelText(tr("Writing to %1...").arg(path.section('/', -1)));
        }
    }

    progDlg.reset();
    QMessageBox::information(this,
                             tr("Extraction done"),
                             tr("%1 sample extraction tasks were completed.").arg(taskList.size()));
}

void MainWindow::on_actionactionExportDdbLayout_triggered()
{
    QMap<uint64_t, QString> layout;

    setCursor(Qt::WaitCursor);
    // Iterate stationaries
    auto stationaryRoot = SearchForChunkByPath({ "voice", "stationary" });
    do {
        if(!stationaryRoot) break;
        // Iterate voice colors
        foreach(auto voiceColor, stationaryRoot->Children) {
            // Iterate stationary segments
            foreach(auto staSeg, voiceColor->Children) {
                // Iterate each pitch of the segment
                foreach(auto pitchSeg, staSeg->Children) {
                    auto frameRefs = pitchSeg->GetChildByName("<Frames>");
                    auto &props = frameRefs->GetPropertiesMap();
                    float pitch;
                    STUFF_INTO(pitchSeg->GetProperty("mPitch").data, pitch, float);
                    QString pitchStr = Common::RelativePitchToNoteName(pitch);
                    for (auto it = props.cbegin(); it != props.cend(); it++) {
                        if (it.key() == "Count") continue;
                        uint64_t addr;
                        STUFF_INTO(it.value().data, addr, uint64_t);
                        layout[addr] = QString("Stationary %1 > %2 (%3) @ %4 %5").arg(
                                        voiceColor->GetName(),
                                        staSeg->GetName(),
                                        pitchSeg->GetName(),
                                        pitchStr,
                                        it.key()
                                    );
                    }

                    // SND
                    uint64_t addr;
                    STUFF_INTO(pitchSeg->GetProperty("SND Sample offset").data, addr, uint64_t);
                    layout[addr] = QString("Stationary %1 > %2 (%3) @ %4 Sound").arg(
                                voiceColor->GetName(),
                                staSeg->GetName(),
                                pitchSeg->GetName(),
                                pitchStr
                            );
                }
            }
        }
    } while(0);

    // Iterate articulations
    auto articulationRoot = SearchForChunkByPath({ "voice", "articulation" });
    do {
        if(!articulationRoot) break;
        // Iterate beginPhonemes
        foreach(auto beginPhoneme, articulationRoot->Children) {
            // Iterate end phonemes
            foreach(auto endPhoneme, beginPhoneme->Children) {
                // Triphonemes
                if(endPhoneme->ObjectSignature() == "ART ") {
                    foreach(auto thirdPhoneme, endPhoneme->Children) {
                        foreach(auto pitchSeg, thirdPhoneme->Children) {
                            auto frameRefs = pitchSeg->GetChildByName("<Frames>");
                            auto &props = frameRefs->GetPropertiesMap();
                            float pitch;
                            STUFF_INTO(pitchSeg->GetProperty("mPitch").data, pitch, float);
                            QString pitchStr = Common::RelativePitchToNoteName(pitch);
                            for (auto it = props.cbegin(); it != props.cend(); it++) {
                                if (it.key() == "Count") continue;
                                uint64_t addr;
                                STUFF_INTO(it.value().data, addr, uint64_t);
                                layout[addr] = QString("Triphone Articulation %1 > %2 @ %3 %4").arg(
                                                pitchSeg->GetName(),
                                                QString("[%1 ~ %2 ~ %3]").arg(
                                                    beginPhoneme->GetName(),
                                                    endPhoneme->GetName(),
                                                    thirdPhoneme->GetName()),
                                                pitchStr,
                                                it.key()
                                            );
                            }

                            // SND
                            uint64_t addr;
                            STUFF_INTO(pitchSeg->GetProperty("SND Sample offset").data, addr, uint64_t);
                            layout[addr] = QString("Triphone Articulation %1 > %2 @ %3 Sound").arg(
                                            pitchSeg->GetName(),
                                            QString("[%1 ~ %2 ~ %3]").arg(
                                                beginPhoneme->GetName(),
                                                endPhoneme->GetName(),
                                                thirdPhoneme->GetName()),
                                            pitchStr
                                        );
                        }
                    }
                    continue;
                }

                // Iterate each pitch of the segment
                foreach(auto pitchSeg, endPhoneme->Children) {
                    auto frameRefs = pitchSeg->GetChildByName("<Frames>");
                    auto &props = frameRefs->GetPropertiesMap();
                    float pitch;
                    STUFF_INTO(pitchSeg->GetProperty("mPitch").data, pitch, float);
                    QString pitchStr = Common::RelativePitchToNoteName(pitch);
                    for (auto it = props.cbegin(); it != props.cend(); it++) {
                        if (it.key() == "Count") continue;
                        uint64_t addr;
                        STUFF_INTO(it.value().data, addr, uint64_t);
                        layout[addr] = QString("Articulation %1 > %2 @ %3 %4").arg(
                                        pitchSeg->GetName(),
                                        QString("[%1 ~ %2]").arg(beginPhoneme->GetName(), endPhoneme->GetName()),
                                        pitchStr,
                                        it.key()
                                    );
                    }

                    // SND
                    uint64_t addr;
                    STUFF_INTO(pitchSeg->GetProperty("SND Sample offset").data, addr, uint64_t);
                    layout[addr] = QString("Articulation %1 > %2 @ %3 Sound").arg(
                                    pitchSeg->GetName(),
                                    QString("[%1 ~ %2]").arg(beginPhoneme->GetName(), endPhoneme->GetName()),
                                    pitchStr
                                );
                }
            }
        }
    } while(0);
    setCursor(Qt::ArrowCursor);

    // Export as CSV
    QFileDialog filedlg;
    QString filename;

    filename = filedlg.getSaveFileName(
            this,
            tr("Save CSV..."),
            QDir::currentPath(),
            "Comma separated values (*.csv)");

    if(filename.isEmpty())
        return;

    QFile file(filename);
    file.open(QFile::WriteOnly);
    if(!file.isWritable()) {
        QMessageBox::critical(this, tr("Failed to export DDB Layout"), tr("File is not writable"));
        return;
    }

    QTextStream stream(&file);

    for (auto it = layout.cbegin(); it != layout.cend(); it++) {
        QString val = it.value();
        stream << '\'' << QString::number(it.key(), 16) << "," << val.replace(QLatin1String(","), QLatin1String("\\,")) << ",\n";
    }

    QMessageBox::information(this, tr("Export finished"), tr("A total of %1 records exported.").arg(layout.size()));
}


void MainWindow::on_actionPack_DevDB_triggered()
{
    if (!mDdiPath.endsWith(".tree")) {
        QMessageBox::critical(this, "Cannot pack DB", "Pack DB is only intended for development DBs");
        return;
    }

    // Sanity check the filesystem structure
    const QString devDbFsRoot = mDdiPath.section('.', 0, -2) + '/';
    size_t idx = 0;
    QProgressDialog progDlg(QString(),
                            QString(),
                            0,
                            1,
                            this);
    progDlg.setWindowModality(Qt::WindowModal);
    progDlg.setMinimumDuration(0);
    progDlg.setAutoReset(false);
    progDlg.setMaximum(0);
    progDlg.setValue(0);
    progDlg.setWindowTitle(tr("Sanity check on filesystem"));
    progDlg.show();

    if (QMessageBox::question(this, "Check DB", "Do you wish to check for DB consistency before packing?") == QMessageBox::Yes)
    {
        LeadingQwordGuard qwg(false);
        // Stationaries
        auto stationaryRoot = SearchForChunkByPath({ "voice", "stationary" });

        // Iterate voice colors
        foreach(auto voiceColor, stationaryRoot->Children) {
            // Iterate stationary segments
            foreach(auto staSeg, voiceColor->Children) {
                // Iterate each pitch of the segment
                foreach(auto pitchSeg, staSeg->Children) {
                    QString targetFile = devDbFsRoot
                                         + QString("voice/stationary/%1/%2/%3")
                                               .arg(voiceColor->GetName(),
                                                    Common::devDbDirEncode(staSeg->GetName()),
                                                    Common::devDbDirEncode(pitchSeg->GetName()));
                    progDlg.setLabelText(targetFile);
                    progDlg.setValue(idx++);

                    if (!QFile::exists(targetFile)) {
                        QMessageBox::critical(this, "Stationary check fail", "Cannot find " + targetFile);
                        return;
                    }

                    FILE* f = fopen(targetFile.toLatin1(), "rb");
                    if (!f) {
                        QMessageBox::warning(this, "Cannot open " + targetFile, QString("Error %1").arg(errno));
                        continue;
                    }
                    auto STAp = new ChunkDBVStationaryPhUPart_DevDB;
                    STAp->Read(f);
                    fclose(f);
                    if (STAp->GetProperty("Frame count").data != pitchSeg->GetProperty("Frame count").data) {
                        QMessageBox::critical(this, "Stationary check fail", staSeg->GetName() + " frame count mismatch");
                        delete STAp;
                        return;
                    }
                    delete STAp;
                }
            }
        }

        // Articulation
        auto articulationRoot = SearchForChunkByPath({ "voice", "articulation" });        // Iterate beginPhonemes
        foreach(auto beginPhoneme, articulationRoot->Children) {
            // Iterate end phonemes
            foreach(auto endPhoneme, beginPhoneme->Children) {
                // Triphonemes
                if(endPhoneme->ObjectSignature() == "ART ") {
                    foreach(auto thirdPhoneme, endPhoneme->Children) {
                        QString targetFile = devDbFsRoot
                                             + QString("voice/articulation/%1/%2/%3")
                                                   .arg(Common::devDbDirEncode(beginPhoneme->GetName()),
                                                        Common::devDbDirEncode(endPhoneme->GetName()),
                                                        Common::devDbDirEncode(thirdPhoneme->GetName()));
                        progDlg.setLabelText(targetFile);
                        progDlg.setValue(idx++);

                        if (!QFile::exists(targetFile) && !QFile::exists(targetFile += ".part")) {
                            QMessageBox::critical(this, "Articulation triphoneme check fail", "Cannot find " + targetFile);
                            return;
                        }

                        FILE* f = fopen(targetFile.toLatin1(), "rb");
                        if (!f) {
                            QMessageBox::warning(this, "Cannot open " + targetFile, QString("Error %1").arg(errno));
                            continue;
                        }
                        auto ARTu = new ChunkDBVArticulationPhU_DevDB;
                        ARTu->Read(f);
                        fclose(f);

                        for(auto pitch = 0; pitch < thirdPhoneme->Children.size(); pitch++) {
                            auto pitchSeg = thirdPhoneme->Children[pitch];

                            if (ARTu->Children[pitch]->GetProperty("Frame count").data != pitchSeg->GetProperty("Frame count").data) {
                                QMessageBox::critical(this,
                                                      "Articulation triphoneme check fail",
                                                      QString("%1-%2-%3 %4 frame count mismatch")
                                                          .arg(beginPhoneme->GetName(),
                                                               endPhoneme->GetName(),
                                                               thirdPhoneme->GetName(),
                                                               QString::number(pitch)));
                                delete ARTu;
                                return;
                            }
                        }

                        delete ARTu;
                    }
                    continue;
                }

                QString targetFile = devDbFsRoot
                                     + QString("voice/articulation/%1/%2")
                                           .arg(Common::devDbDirEncode(beginPhoneme->GetName()),
                                                Common::devDbDirEncode(endPhoneme->GetName()));
                progDlg.setLabelText(targetFile);
                progDlg.setValue(idx++);

                if (!QFile::exists(targetFile) && !QFile::exists(targetFile += ".part")) {
                    QMessageBox::critical(this, "Articulation check fail", "Cannot find " + targetFile);
                    return;
                }

                FILE* f = fopen(targetFile.toLatin1(), "rb");
                if (!f) {
                    QMessageBox::warning(this, "Cannot open " + targetFile, QString("Error %1").arg(errno));
                    continue;
                }
                auto ARTu = new ChunkDBVArticulationPhU_DevDB;
                ARTu->Read(f);
                fclose(f);

                // Iterate each pitch of the segment
                if (ARTu->Children.count() != endPhoneme->Children.count()) {
                    QMessageBox::critical(this,
                                          "Articulation check fail",
                                          QString("%1-%2 inconsistent sample count (Tree %3 Item %4)")
                                              .arg(beginPhoneme->GetName(),
                                                   endPhoneme->GetName())
                                              .arg(endPhoneme->Children.count())
                                              .arg(ARTu->Children.count())
                                          );
                    delete ARTu;
                    return;
                }

                for(auto pitch = 0; pitch < endPhoneme->Children.size(); pitch++) {
                    auto pitchSeg = endPhoneme->Children[pitch];

                    if (ARTu->Children[pitch]->GetProperty("Frame count").data != pitchSeg->GetProperty("Frame count").data) {
                        QMessageBox::critical(this,
                                              "Articulation check fail",
                                              QString("%1-%2 %3 frame count mismatch")
                                                  .arg(beginPhoneme->GetName(),
                                                       endPhoneme->GetName(),
                                                       QString::number(pitch)));
                        delete ARTu;
                        return;
                    }
                }

                delete ARTu;
            }
        }

        QMessageBox::information(this, "Consistency check pass", "All required files exists and DB index tree is consistent.");
    }


    QFileDialog filedlg;
    QString outputDir;

    outputDir = filedlg.getExistingDirectory(
        this,
        tr("Save database..."),
        QDir::currentPath());
    if(outputDir.isEmpty())
        return;

    QString targetFile = outputDir + '/' + mDdiPath.section('/', -1).section('.', 0, -2) + ".ddi";
    QFile ddi(targetFile);
    if (ddi.exists()) {
        if (QMessageBox::question(this, "DDI Exists", ddi.fileName() + " exists. Delete it?\n"
                                                                       "Otherwise packing would abort.") == QMessageBox::Yes) {
            ddi.remove();
        } else {
            return;
        }
    }
    if (!QFile::copy(mDdiPath, targetFile)) {
        QMessageBox::critical(this, "Cannot copy file", "Cannot copy database tree to " + targetFile);
        return;
    }

    if (!ddi.open(QFile::ReadWrite)) {
        QMessageBox::critical(this, "Cannot write file", "Cannot open target DDI " + targetFile);
        return;
    }

    QFile ddb(targetFile.section('.', 0, -2) + ".ddb");
    if (ddb.exists() &&
        QMessageBox::question(this, "DDB Exists", ddb.fileName() + " exists. Delete it?") == QMessageBox::Yes) {
        ddb.remove();
    }
    if (!ddb.open(QFile::ReadWrite)) {
        QMessageBox::critical(this, "Cannot write file", "Cannot open target DDB " + targetFile);
        return;
    }

    progDlg.reset();
    progDlg.setWindowTitle("Pack DB");
    progDlg.setMaximum(idx);
    idx = 0;
    {
        LeadingQwordGuard qwg(false);
        // Writer convenience function
        auto writeOffset = [&](size_t writeToOffset, size_t offset) -> QByteArray {
            auto ddiPtr = QByteArray((const char*)&offset, sizeof(offset));
            ddi.seek(writeToOffset);
            ddi.write(ddiPtr);
            return ddiPtr;
        };
        auto write32 = [&](size_t writeToOffset, uint32_t data) -> QByteArray {
            auto ddiPtr = QByteArray((const char*)&data, sizeof(data));
            ddi.seek(writeToOffset);
            ddi.write(ddiPtr);
            return ddiPtr;
        };

        auto writeBlock = [&](QByteArray content, size_t ddiOffset, int offsetOffset = 0) -> size_t {
            auto offset = ddb.pos();
            ddb.write(content);
            writeOffset(ddiOffset, offset + offsetOffset);
            return offset;
        };

        // Stationaries
        auto stationaryRoot = SearchForChunkByPath({ "voice", "stationary" });

        // Iterate voice colors
        foreach(auto voiceColor, stationaryRoot->Children) {
            // Iterate stationary segments
            foreach(auto staSeg, voiceColor->Children) {
                // Iterate each pitch of the segment
                foreach(auto pitchSeg, staSeg->Children) {
                    QString targetFile = devDbFsRoot
                                         + QString("voice/stationary/%1/%2/%3")
                                               .arg(voiceColor->GetName(),
                                                    Common::devDbDirEncode(staSeg->GetName()),
                                                    Common::devDbDirEncode(pitchSeg->GetName()));
                    progDlg.setLabelText(targetFile);
                    progDlg.setValue(idx++);

                    if (!QFile::exists(targetFile)) {
                        QMessageBox::critical(this, "Stationary check fail", "Cannot find " + targetFile);
                        return;
                    }

                    FILE* f = fopen(targetFile.toLatin1(), "rb");
                    if (!f) {
                        QMessageBox::warning(this, "Cannot open " + targetFile, QString("Error %1").arg(errno));
                        continue;
                    }
                    auto STAp = new ChunkDBVStationaryPhUPart_DevDB;
                    STAp->Read(f);
                    fclose(f);

                    // Write SMS Frames
                    auto framesDir = pitchSeg->GetChildByName("<Frames>"); assert(framesDir);
                    auto framesProps = framesDir->GetPropertiesMap(); framesProps.remove("Count");
                    foreach (auto i, framesProps) {
                        auto frame = STAp->framesToWrite.takeFirst(); assert(frame->ObjectSignature() == "FRM2");
                        writeBlock(((ChunkSMSFrameChunk*)frame)->rawData, i.offset);
                    }
                    float relPitch; STUFF_INTO(STAp->GetProperty("mPitch").data, relPitch, float);
                    qDebug() << staSeg->GetName() << Common::RelativePitchToNoteName(relPitch);
                    // Write sound chunk
                    {
                        auto snd = (ChunkSoundChunk*)(STAp->GetChildByName("SND"));
                        //double samplePerFrame = (double)snd->sampleCount / (double)STAp->allFramesCount;
                        size_t samplePerFrame = 256;
//                        auto optimizedFrom = snd->OptimizedFrom(samplePerFrame * (STAp->skipFrameCount + 1), STAp->GetProperty("mPitch").data);
                        auto optimizedFrom = samplePerFrame * (STAp->skipFrameCount + 1);
//                        writeBlock(snd->GetTruncatedChunk(optimizedFrom - 0x400,
//                                                          optimizedFrom + samplePerFrame * STAp->frameCount + 0x400),
//                                   pitchSeg->GetProperty("SND Sample offset").offset, 0x12 + 0x800);
                        writeBlock(snd->GetTruncatedChunk(optimizedFrom - 0x400,
                                                          optimizedFrom + samplePerFrame * STAp->frameCount + 0x400),
                                   pitchSeg->GetProperty("SND Sample offset").offset, 0x12 + 0x800);
                        write32(pitchSeg->GetProperty("SND Sample count").offset, samplePerFrame * STAp->frameCount + 0x800);
                    }

                    delete STAp;
                }
            }
        }

        // Articulation
        auto articulationRoot = SearchForChunkByPath({ "voice", "articulation" });        // Iterate beginPhonemes
        foreach(auto beginPhoneme, articulationRoot->Children) {
            // Iterate end phonemes
            foreach(auto endPhoneme, beginPhoneme->Children) {
                // Triphonemes
                if(endPhoneme->ObjectSignature() == "ART ") {
                    foreach(auto thirdPhoneme, endPhoneme->Children) {
                        QString targetFile = devDbFsRoot
                                             + QString("voice/articulation/%1/%2/%3")
                                                   .arg(Common::devDbDirEncode(beginPhoneme->GetName()),
                                                        Common::devDbDirEncode(endPhoneme->GetName()),
                                                        Common::devDbDirEncode(thirdPhoneme->GetName()));
                        progDlg.setLabelText(targetFile);
                        progDlg.setValue(idx++);

                        if (!QFile::exists(targetFile) && !QFile::exists(targetFile += ".part")) {
                            QMessageBox::critical(this, "Articulation triphoneme check fail", "Cannot find " + targetFile);
                            return;
                        }

                        FILE* f = fopen(targetFile.toLatin1(), "rb");
                        if (!f) {
                            QMessageBox::warning(this, "Cannot open " + targetFile, QString("Error %1").arg(errno));
                            continue;
                        }
                        auto ARTu = new ChunkDBVArticulationPhU_DevDB;
                        ARTu->Read(f);
                        fclose(f);

                        for(auto pitch = 0; pitch < endPhoneme->Children.size(); pitch++) {
                            auto pitchSeg = endPhoneme->Children[pitch];
                            auto ARTp = (ChunkDBVArticulationPhUPart_DevDB*)(ARTu->Children[pitch]);

                            // Write SMS Frames
                            auto framesDir = pitchSeg->GetChildByName("<Frames>"); assert(framesDir);
                            auto framesProps = framesDir->GetPropertiesMap(); framesProps.remove("Count");
                            foreach (auto i, framesProps) {
                                auto frame = ARTp->framesToWrite.takeFirst(); assert(frame->ObjectSignature() == "FRM2");
                                writeBlock(((ChunkSMSFrameChunk*)frame)->rawData, i.offset);
                            }
                            // Write sound chunk
                            {
                                auto snd = (ChunkSoundChunk*)(ARTp->GetChildByName("SND"));
                                double samplePerFrame = (double)snd->sampleCount / (double)ARTp->allFramesCount;
                                auto sndOffset =
                                    writeBlock(snd->GetTruncatedChunk(samplePerFrame * ARTp->skipFrameCount - 0x400,
                                                                      samplePerFrame * (ARTp->skipFrameCount + ARTp->frameCount) + 0x400),
                                               pitchSeg->GetProperty("SND Sample offset").offset, 0x12);
                                writeOffset(pitchSeg->GetProperty("SND Sample offset+800").offset, sndOffset + 0x12 + 0x800); // FIXME
                                write32(pitchSeg->GetProperty("SND Sample count").offset, samplePerFrame * ARTp->frameCount + 0x800);
                            }
                        }

                        delete ARTu;
                    }
                    continue;
                }

                QString targetFile = devDbFsRoot
                                     + QString("voice/articulation/%1/%2")
                                           .arg(Common::devDbDirEncode(beginPhoneme->GetName()),
                                                Common::devDbDirEncode(endPhoneme->GetName()));
                progDlg.setLabelText(targetFile);
                progDlg.setValue(idx++);

                if (!QFile::exists(targetFile) && !QFile::exists(targetFile += ".part")) {
                    QMessageBox::critical(this, "Articulation check fail", "Cannot find " + targetFile);
                    return;
                }

                FILE* f = fopen(targetFile.toLatin1(), "rb");
                if (!f) {
                    QMessageBox::warning(this, "Cannot open " + targetFile, QString("Error %1").arg(errno));
                    continue;
                }
                auto ARTu = new ChunkDBVArticulationPhU_DevDB;
                ARTu->Read(f);
                fclose(f);

                // Iterate each pitch of the segment
                if (ARTu->Children.count() != endPhoneme->Children.count()) {
                    QMessageBox::critical(this,
                                          "Articulation check fail",
                                          QString("%1-%2 inconsistent sample count (Tree %3 Item %4)")
                                              .arg(beginPhoneme->GetName(),
                                                   endPhoneme->GetName())
                                              .arg(endPhoneme->Children.count())
                                              .arg(ARTu->Children.count())
                                          );
                    delete ARTu;
                    return;
                }

                for(auto pitch = 0; pitch < endPhoneme->Children.size(); pitch++) {
                    auto pitchSeg = endPhoneme->Children[pitch];
                    auto ARTp = (ChunkDBVArticulationPhUPart_DevDB*)(ARTu->Children[pitch]);

                    // Write SMS Frames
                    auto framesDir = pitchSeg->GetChildByName("<Frames>"); assert(framesDir);
                    auto framesProps = framesDir->GetPropertiesMap(); framesProps.remove("Count");
                    foreach (auto i, framesProps) {
                        auto frame = ARTp->framesToWrite.takeFirst(); assert(frame->ObjectSignature() == "FRM2");
                        writeBlock(((ChunkSMSFrameChunk*)frame)->rawData, i.offset);
                    }
                    // Write sound chunk
                    {
                        auto snd = (ChunkSoundChunk*)(ARTp->GetChildByName("SND"));
                        double samplePerFrame = (double)snd->sampleCount / (double)ARTp->allFramesCount;
                        auto optimizedFrom = samplePerFrame * ARTp->skipFrameCount;
                        if (ARTp->IsStartingWithVowel()) {
                            optimizedFrom = snd->OptimizedFrom(samplePerFrame * (ARTp->skipFrameCount + 1), ARTp->GetProperty("mPitch").data);
                        }
                        auto sndOffset =
                            writeBlock(snd->GetTruncatedChunk(optimizedFrom - 0x400,
                                                              optimizedFrom + samplePerFrame * ARTp->frameCount + 0x400),
                                       pitchSeg->GetProperty("SND Sample offset").offset, 0x12);
                        writeOffset(pitchSeg->GetProperty("SND Sample offset+800").offset, sndOffset + 0x12 + 0x800); // FIXME
                        write32(pitchSeg->GetProperty("SND Sample count").offset, samplePerFrame * ARTp->frameCount + 0x800);
                    }
                }

                delete ARTu;
            }
        }

        // Add a hash segment filled with 0x0DD171EA (DDIVIEW)
        ddi.seek(0);
        auto phase1Ddi = ddi.readAll();
        QByteArray finalDdi;

        // Hash is after PHDC, find its end
        auto phdc = SearchForChunkByPath({ "<Phoneme Dictionary>" }); assert(phdc);
        finalDdi.append("\0\0\0\0\0\0\0\0DBSe", 12); // Fix header; DevDB = "DBS ", DB = DBSe
        finalDdi.append(phase1Ddi.left(phdc->GetOriginalOffset() + phdc->GetSize()).mid(12));

        // 260 bytes of filler
        QByteArray filler;
        while (filler.size() < 260) {
            filler.append("\x0d\xd1\x71\xea", 4);
        }
        filler.resize(260);

        finalDdi.append(filler);

        // Add final parts
        finalDdi.append(phase1Ddi.mid(phdc->GetOriginalOffset() + phdc->GetSize()));

        ddi.seek(0);
        ddi.write(finalDdi);

        ddi.close();
        ddb.close();
    }

    QMessageBox::information(this, "Packing done", ddi.fileName() + '\n' + ddb.fileName());
}


void MainWindow::on_listProperties_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    mLblPropertyOffset->setText("PROP " + QString::number(current ?
                                              current->data(BaseChunk::ItemOffsetRole).toULongLong() :
                                                          0
                                                          , 16).toUpper());
}


void MainWindow::on_treeStructureDdb_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    if(!current)
        return;

    auto chunk = current->data(0, BaseChunk::ItemChunkRole).value<BaseChunk*>();
    if (!chunk) {
        return;
    }

    auto props = chunk->GetPropertiesMap();

    if (chunk->GetSignature() == "SND ") {
        // Read sound and put it onto the customplot
        mDdbFile.seek(chunk->GetOriginalOffset() + 0x14);
        mDdbStream.setDevice(&mDdbFile);

        int sampleCount, sampleRate;
        STUFF_INTO(props["Sample count"].data, sampleCount, int);
        STUFF_INTO(props["Sample rate"].data, sampleRate, int);
        QVector<double> keys, vals;
        keys.reserve(sampleCount);
        vals.reserve(sampleCount);
        double sampleToSecFac = 1.0 / sampleRate, sampleValNormFac = 1.0 / 32768.0;
        for (int i = 0; i < sampleCount; i++) {
            int16_t sample;
            mDdbStream >> sample;
            keys.append(i * sampleToSecFac);
            vals.append(sample * sampleValNormFac);
        }
        mWaveformPlot->xAxis->setRange(0.0, sampleCount * sampleToSecFac);
        mWaveformGraph->setData(keys, vals, true);
        mWaveformPlot->replot();
    }
}

