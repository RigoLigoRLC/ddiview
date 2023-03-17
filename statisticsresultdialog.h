#ifndef STATISTICSRESULTDIALOG_H
#define STATISTICSRESULTDIALOG_H

#include <QDialog>
#include <QMap>

namespace Ui {
class StatisticsResultDialog;
}

class StatisticsResultDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StatisticsResultDialog(QWidget *parent = nullptr);
    ~StatisticsResultDialog();

    void SetResult(QMap<QByteArray, uint32_t> results, uint32_t nonExistent);

private slots:
    void on_comboPropFmt_currentIndexChanged(int index);

private:
    void InitUI();
    void TriggerDataReformat();

    Ui::StatisticsResultDialog *ui;

    QMap<QByteArray, uint32_t> mResults;
    uint32_t mNonExistent;
};

#endif // STATISTICSRESULTDIALOG_H
