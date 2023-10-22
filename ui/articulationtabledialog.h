#ifndef ARTICULATIONTABLEDIALOG_H
#define ARTICULATIONTABLEDIALOG_H

#include <QDialog>

namespace Ui {
class ArticulationTableDialog;
}

class ArticulationTableDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ArticulationTableDialog(QWidget *parent = nullptr);
    ~ArticulationTableDialog();

    void SetDiphonemeMatrix(QStringList phonemeList, const QVector<int>&);

private slots:
    void on_btnExportCsv_clicked();

private:
    Ui::ArticulationTableDialog *ui;

    QVector<int> m_supportMatrix;
    QStringList m_phonemes;
};

#endif // ARTICULATIONTABLEDIALOG_H
