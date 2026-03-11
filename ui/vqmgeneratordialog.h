#ifndef VQMGENERATORDIALOG_H
#define VQMGENERATORDIALOG_H

#include <QDialog>

namespace Ui {
class VqmGeneratorDialog;
}

class VqmGeneratorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VqmGeneratorDialog(QWidget *parent = nullptr);
    ~VqmGeneratorDialog();

    QString GetWavPath() const;
    QString GetOutputDir() const;
    QString GetSampleName() const;
    double GetPitch() const;
    double GetBeginTime() const;
    double GetEndTime() const;
    int GetFrameRate() const;
    int GetMaxHarmonics() const;

private slots:
    void on_btnBrowseWav_clicked();
    void on_btnBrowseOutput_clicked();
    void updatePreview();

private:
    bool loadWavInfo(const QString& path);

private:
    Ui::VqmGeneratorDialog *ui;

    // WAV file info
    int mSampleRate;
    int mChannels;
    int mBitsPerSample;
    double mDuration;
};

#endif // VQMGENERATORDIALOG_H
