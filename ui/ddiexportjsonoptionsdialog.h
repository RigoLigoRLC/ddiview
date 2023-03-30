#ifndef DDIEXPORTJSONOPTIONSDIALOG_H
#define DDIEXPORTJSONOPTIONSDIALOG_H

#include <QDialog>

namespace Ui {
class DdiExportJsonOptionsDialog;
}

class DdiExportJsonOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DdiExportJsonOptionsDialog(QWidget *parent = nullptr);
    ~DdiExportJsonOptionsDialog();

    bool GetDoUseVerbatimPropValues();
    bool GetDoUseCompactJson();

private:
    Ui::DdiExportJsonOptionsDialog *ui;
};

#endif // DDIEXPORTJSONOPTIONSDIALOG_H
