#include "statisticsresultdialog.h"
#include "chunk/basechunk.h"
#include "ui_statisticsresultdialog.h"
#include "chunk/propertytype.h"

StatisticsResultDialog::StatisticsResultDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StatisticsResultDialog)
{
    ui->setupUi(this);

    InitUI();
}

StatisticsResultDialog::~StatisticsResultDialog()
{
    delete ui;
}

void StatisticsResultDialog::SetResult(QMap<QByteArray, uint32_t> results, uint32_t nonExistent)
{
    mResults = results;
    mNonExistent = nonExistent;
    TriggerDataReformat();
}

void StatisticsResultDialog::InitUI()
{
    for(int i = 0; i < PropertyTypeCount; i++) {
        ui->comboPropFmt->addItem(PropertyTypeNames[i]);
    }
}

void StatisticsResultDialog::TriggerDataReformat()
{
    PropertyType type = static_cast<PropertyType>(ui->comboPropFmt->currentIndex());
    ui->treeWidget->clear();

    for(auto i = mResults.cbegin(); i != mResults.cend(); i++) {
        QString fmt = FormatProperty(ChunkProperty {i.key(), type});
        ui->treeWidget->addTopLevelItem(
                    new QTreeWidgetItem(QStringList { fmt, QString::number(i.value()) })
                    );
    }

    ui->treeWidget->addTopLevelItem(
                new QTreeWidgetItem(QStringList { tr("[No such property]"), QString::number(mNonExistent) })
                );
}

void StatisticsResultDialog::on_comboPropFmt_currentIndexChanged(int index)
{
    TriggerDataReformat();
}

