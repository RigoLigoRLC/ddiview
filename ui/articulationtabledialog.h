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

    void SetSupportMatrix(QStringList phonemeList, const QVector<int>&);

private:
    Ui::ArticulationTableDialog *ui;
};

#endif // ARTICULATIONTABLEDIALOG_H
