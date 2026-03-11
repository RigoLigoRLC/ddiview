#include "vqmgeneratordialog.h"
#include "ui_vqmgeneratordialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QDataStream>

VqmGeneratorDialog::VqmGeneratorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VqmGeneratorDialog),
    mSampleRate(44100),
    mChannels(1),
    mBitsPerSample(16),
    mDuration(0.0)
{
    ui->setupUi(this);

    connect(ui->edtSampleName, &QLineEdit::textChanged, this, &VqmGeneratorDialog::updatePreview);
    connect(ui->spnPitch, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VqmGeneratorDialog::updatePreview);
    connect(ui->spnBegin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VqmGeneratorDialog::updatePreview);
    connect(ui->spnEnd, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VqmGeneratorDialog::updatePreview);
    connect(ui->edtOutputDir, &QLineEdit::textChanged, this, &VqmGeneratorDialog::updatePreview);

    updatePreview();
}

VqmGeneratorDialog::~VqmGeneratorDialog()
{
    delete ui;
}

QString VqmGeneratorDialog::GetWavPath() const
{
    return ui->edtWavPath->text();
}

QString VqmGeneratorDialog::GetOutputDir() const
{
    return ui->edtOutputDir->text();
}

QString VqmGeneratorDialog::GetSampleName() const
{
    return ui->edtSampleName->text();
}

double VqmGeneratorDialog::GetPitch() const
{
    return ui->spnPitch->value();
}

double VqmGeneratorDialog::GetBeginTime() const
{
    return ui->spnBegin->value();
}

double VqmGeneratorDialog::GetEndTime() const
{
    return ui->spnEnd->value();
}

int VqmGeneratorDialog::GetFrameRate() const
{
    return ui->spnFrameRate->value();
}

int VqmGeneratorDialog::GetMaxHarmonics() const
{
    return ui->spnHarmonics->value();
}

void VqmGeneratorDialog::on_btnBrowseWav_clicked()
{
    QString path = QFileDialog::getOpenFileName(this,
        tr("Select WAV File"),
        QString(),
        tr("WAV Files (*.wav);;All Files (*)"));

    if (!path.isEmpty()) {
        if (loadWavInfo(path)) {
            ui->edtWavPath->setText(path);

            // Auto-fill sample name from filename
            QFileInfo fi(path);
            ui->edtSampleName->setText(fi.baseName());

            // Set end time to duration
            ui->spnEnd->setValue(mDuration);

            updatePreview();
        }
    }
}

void VqmGeneratorDialog::on_btnBrowseOutput_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        tr("Select Output Directory"),
        QString(),
        QFileDialog::ShowDirsOnly);

    if (!dir.isEmpty()) {
        ui->edtOutputDir->setText(dir);
        updatePreview();
    }
}

bool VqmGeneratorDialog::loadWavInfo(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Error"), tr("Cannot open WAV file."));
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    // Read RIFF header
    char riff[4];
    file.read(riff, 4);
    if (strncmp(riff, "RIFF", 4) != 0) {
        QMessageBox::warning(this, tr("Error"), tr("Invalid WAV file: missing RIFF header."));
        return false;
    }

    quint32 fileSize;
    stream >> fileSize;

    char wave[4];
    file.read(wave, 4);
    if (strncmp(wave, "WAVE", 4) != 0) {
        QMessageBox::warning(this, tr("Error"), tr("Invalid WAV file: missing WAVE format."));
        return false;
    }

    // Find fmt chunk
    bool foundFmt = false;
    quint32 dataSize = 0;

    while (!file.atEnd()) {
        char chunkId[4];
        quint32 chunkSize;
        file.read(chunkId, 4);
        stream >> chunkSize;

        if (strncmp(chunkId, "fmt ", 4) == 0) {
            quint16 audioFormat;
            quint16 numChannels;
            quint32 sampleRate;
            quint32 byteRate;
            quint16 blockAlign;
            quint16 bitsPerSample;

            stream >> audioFormat >> numChannels >> sampleRate >> byteRate >> blockAlign >> bitsPerSample;

            if (audioFormat != 1) {
                QMessageBox::warning(this, tr("Error"), tr("Only PCM WAV files are supported."));
                return false;
            }

            mSampleRate = sampleRate;
            mChannels = numChannels;
            mBitsPerSample = bitsPerSample;
            foundFmt = true;

            // Skip rest of fmt chunk
            if (chunkSize > 16) {
                file.seek(file.pos() + chunkSize - 16);
            }
        } else if (strncmp(chunkId, "data", 4) == 0) {
            dataSize = chunkSize;
            break;
        } else {
            // Skip unknown chunk
            file.seek(file.pos() + chunkSize);
        }
    }

    if (!foundFmt || dataSize == 0) {
        QMessageBox::warning(this, tr("Error"), tr("Invalid WAV file: missing fmt or data chunk."));
        return false;
    }

    // Calculate duration
    int bytesPerSample = mBitsPerSample / 8;
    int samplesPerFrame = mChannels * bytesPerSample;
    quint32 totalSamples = dataSize / samplesPerFrame;
    mDuration = (double)totalSamples / mSampleRate;

    return true;
}

void VqmGeneratorDialog::updatePreview()
{
    QString outputDir = ui->edtOutputDir->text();
    QString sampleName = ui->edtSampleName->text();

    if (outputDir.isEmpty() || sampleName.isEmpty()) {
        ui->txtPreview->setText(tr("Please fill in all required fields."));
        return;
    }

    QString preview;
    preview += tr("Output files:\n");
    preview += QString("  %1/VQM/%2.wav\n").arg(outputDir, sampleName);
    preview += QString("  %1/VQM/%2.sms\n").arg(outputDir, sampleName);
    preview += QString("  %1/vqm.ini\n").arg(outputDir);

    ui->txtPreview->setText(preview);
}
