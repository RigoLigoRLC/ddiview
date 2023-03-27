#include "articulationtabledialog.h"
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
}
