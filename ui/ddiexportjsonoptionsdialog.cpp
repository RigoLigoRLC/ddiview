#include "ddiexportjsonoptionsdialog.h"
#include "ui_ddiexportjsonoptionsdialog.h"

DdiExportJsonOptionsDialog::DdiExportJsonOptionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DdiExportJsonOptionsDialog)
{
    ui->setupUi(this);
}

DdiExportJsonOptionsDialog::~DdiExportJsonOptionsDialog()
{
    delete ui;
}

bool DdiExportJsonOptionsDialog::GetDoUseVerbatimPropValues()
{
    return ui->radPropValVerbatim->isChecked();
}

bool DdiExportJsonOptionsDialog::GetDoUseCompactJson()
{
    return !ui->chkJsonFormatted->isChecked();
}
