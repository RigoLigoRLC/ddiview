#ifndef SMSGENERATOR_H
#define SMSGENERATOR_H

#include <QString>
#include <QVector>
#include <cstdint>
#include <cmath>

// SMS file format constants
constexpr uint32_t SMS_MAGIC_SMS2 = 0x32534D53; // "SMS2"
constexpr uint32_t SMS_MAGIC_GEN  = 0x204E4547; // "GEN "
constexpr uint32_t SMS_MAGIC_GTRK = 0x4B525447; // "GTRK"
constexpr uint32_t SMS_MAGIC_RGN  = 0x204E4752; // "RGN "
constexpr uint32_t SMS_MAGIC_FRM2 = 0x324D5246; // "FRM2"
constexpr uint32_t SMS_MAGIC_ENV  = 0x20564E45; // "ENV "

// Frame flags (matching real VOCALOID growl format: 0x000E2287)
constexpr uint64_t FRAME_FLAG_AMPLITUDE      = 0x01;
constexpr uint64_t FRAME_FLAG_FREQUENCY      = 0x02;
constexpr uint64_t FRAME_FLAG_PHASE          = 0x04;
constexpr uint64_t FRAME_FLAG_NOISE_AMP      = 0x10;
constexpr uint64_t FRAME_FLAG_NOISE_PHASE    = 0x20;
constexpr uint64_t FRAME_FLAG_ENVELOPE       = 0x80;
constexpr uint64_t FRAME_FLAG_ENVELOPE2      = 0x100;
constexpr uint64_t FRAME_FLAG_F0             = 0x200;
constexpr uint64_t FRAME_FLAG_GROWL_PARAMS   = 0x2000;
constexpr uint64_t FRAME_FLAG_ENVELOPE3      = 0x20000;
constexpr uint64_t FRAME_FLAG_EXTRA_BYTE     = 0x40000;
constexpr uint64_t FRAME_FLAG_EXTRA_FLOAT    = 0x80000;
constexpr uint64_t FRAME_FLAG_F0_CENTS       = 0x80000000;
constexpr uint64_t FRAME_FLAG_COMPRESSED     = 0x40000000;

// Standard growl frame flags (from real VOCALOID data)
constexpr uint64_t FRAME_FLAGS_GROWL = FRAME_FLAG_AMPLITUDE | FRAME_FLAG_FREQUENCY |
                                       FRAME_FLAG_PHASE | FRAME_FLAG_ENVELOPE |
                                       FRAME_FLAG_F0 | FRAME_FLAG_GROWL_PARAMS |
                                       FRAME_FLAG_ENVELOPE3 | FRAME_FLAG_EXTRA_BYTE |
                                       FRAME_FLAG_EXTRA_FLOAT;
// = 0x000E2287

// Standard harmonic slot count in VOCALOID SMS
constexpr int SMS_HARMONIC_SLOTS = 350;

// Track flags
constexpr uint32_t TRACK_FLAG_SAMPLERATE = 0x01;
constexpr uint32_t TRACK_FLAG_DURATION   = 0x02;
constexpr uint32_t TRACK_FLAG_FRAMERATE  = 0x04;
constexpr uint32_t TRACK_FLAG_PRECISION  = 0x08;

struct SMSHarmonic {
    float frequency;  // Hz
    float amplitude;  // dB
    float phase;      // radians
};

struct SMSFrame {
    int index;
    double duration;
    float f0;         // fundamental frequency in Hz
    QVector<SMSHarmonic> harmonics;
    QVector<float> noiseAmplitudes;  // dB per noise band
    QVector<float> noisePhases;      // radians per noise band
};

struct SMSRegion {
    double duration;   // Region duration in seconds (used by engine for cumulative track duration)
    uint8_t regionType;
    QVector<SMSFrame> frames;
};

struct SMSTrack {
    int trackIndex;
    uint32_t sampleRate;
    double duration;
    uint32_t frameRate;
    uint8_t precision;
    QVector<SMSRegion> regions;
};

class SmsGenerator
{
public:
    SmsGenerator();

    // Load WAV file and perform spectral analysis
    bool analyzeWav(const QString& wavPath, int frameRate, int maxHarmonics,
                    double beginTime = 0.0, double endTime = -1.0);

    // Write SMS file
    bool writeSms(const QString& smsPath);

    // Write VQM.ini file
    static bool writeVqmIni(const QString& iniPath, const QString& sampleName,
                            double begin, double end, double pitch, const QString& wavFilename);

    // Copy WAV file to output directory
    static bool copyWav(const QString& srcPath, const QString& dstPath);

    // Get analysis results
    const SMSTrack& getTrack() const { return mTrack; }

    // Get error message
    QString getError() const { return mError; }

private:
    // Read WAV file
    bool readWav(const QString& path, QVector<float>& samples, int& sampleRate,
                 double beginTime, double endTime);

    // Perform FFT-based harmonic analysis on a frame
    void analyzeFrame(const QVector<float>& samples, int startSample, int windowSize,
                      int sampleRate, int maxHarmonics, SMSFrame& frame);

    // Simple DFT for harmonic extraction (no external FFT library needed)
    void computeDFT(const QVector<float>& samples, int startSample, int windowSize,
                    QVector<float>& magnitudes, QVector<float>& phases);

    // Estimate fundamental frequency using autocorrelation
    float estimateF0(const QVector<float>& samples, int startSample, int windowSize, int sampleRate);

    // Convert frequency to cents (relative to A4=440Hz)
    static float freqToCents(float freq);

    // Apply Hann window
    static float hannWindow(int n, int N);

    // Write growl parameters block (flag 0x2000)
    void writeGrowlBlock(QDataStream& stream, const SMSFrame& frame);

    // Write spectral envelope CChunk (ENV)
    void writeSpectralEnvelope(QDataStream& stream,
                               const SMSFrame& frame, float nyquist, int maxPoints);

private:
    SMSTrack mTrack;
    QString mError;
};

#endif // SMSGENERATOR_H
