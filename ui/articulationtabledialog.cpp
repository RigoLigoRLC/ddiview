#include "articulationtabledialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include "ui_articulationtabledialog.h"
#include "uicommon.h"

ArticulationTableDialog::ArticulationTableDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ArticulationTableDialog)
{
    ui->setupUi(this);
}

ArticulationTableDialog::~ArticulationTableDialog()
{
    delete ui;
}

void ArticulationTableDialog::SetSupportMatrix(QStringList phonemeList, const QVector<int> &supportMatrix)
{
    auto size = phonemeList.size();

    ui->tableArticulation->setRowCount(size);
    ui->tableArticulation->setColumnCount(size);
    ui->tableArticulation->setVerticalHeaderLabels(phonemeList);
    ui->tableArticulation->setHorizontalHeaderLabels(phonemeList);
    ui->tableArticulation->verticalHeader()->setMinimumSectionSize(-1);
    ui->tableArticulation->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Fixed);
    ui->tableArticulation->verticalHeader()->setDefaultSectionSize(10 * UiCommon::SystemScalingFactor());
    ui->tableArticulation->horizontalHeader()->setDefaultSectionSize(32);

    auto newfont = ui->tableArticulation->font();
    newfont.setPixelSize(10 * UiCommon::SystemScalingFactor());
    ui->tableArticulation->setFont(newfont);
    ui->tableArticulation->verticalHeader()->setFont(newfont);

    for(auto i = 0; i < size; i++) {
        for(auto j = 0; j < size; j++) {
            auto x = supportMatrix[i * size + j];
            ui->tableArticulation->setItem(
                        i, j, new QTableWidgetItem(
                            (x == -2 ? "--" :
                             (x == -1 ? "X" :
                              QString("%1").arg(x)
                             )
                            )
                            ));
        }
    }

    m_phonemes = phonemeList;
    m_supportMatrix = supportMatrix;
}

void ArticulationTableDialog::on_btnExportCsv_clicked()
{
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
        QMessageBox::critical(this, tr("Failed to export articulations"), tr("File is not writable"));
        return;
    }

    QTextStream stream(&file);

    stream << ",";

    // Escape commas
    auto phonemes = m_phonemes;
    for(auto &i : phonemes) {
        i.replace(QLatin1String(","), QLatin1String("\\,"));
        stream << i << ',';
    }
    stream << '\n';

    for(int i = 0; i < phonemes.size(); i++) {
        stream << phonemes[i] << ',';
        for(int j = 0; j < phonemes.size(); j++) {
            stream << m_supportMatrix[i * phonemes.size() + j] << ',';
        }
        stream << '\n';
    }

    file.close();
}

