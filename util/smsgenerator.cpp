#include "smsgenerator.h"

#include <QFile>
#include <QDataStream>
#include <QFileInfo>
#include <QDir>
#include <QPair>
#include <QTextStream>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SmsGenerator::SmsGenerator()
{
}

bool SmsGenerator::analyzeWav(const QString& wavPath, int frameRate, int maxHarmonics,
                               double beginTime, double endTime)
{
    QVector<float> samples;
    int sampleRate;

    if (!readWav(wavPath, samples, sampleRate, beginTime, endTime)) {
        return false;
    }

    if (samples.isEmpty()) {
        mError = "No samples to analyze";
        return false;
    }

    // Calculate frame parameters
    // CRITICAL: The DSE4 engine hardcodes frame access at 256 samples per frame.
    // VQM init (sub_1000A2A0) computes frame indices as: time * 44100 / 256
    // and directly indexes the SMS frame array. The SMS MUST have one frame
    // per 256 audio samples, regardless of the user-requested frame rate.
    // The user's frameRate is ignored; we always use sampleRate/256.
    int samplesPerFrame = 256;
    int windowSize = samplesPerFrame * 2; // Overlap
    int totalFrames = samples.size() / samplesPerFrame;

    if (totalFrames <= 0) {
        mError = "Audio too short for analysis";
        return false;
    }

    double duration = (double)samples.size() / sampleRate;

    // Initialize track
    mTrack.trackIndex = 0;
    mTrack.sampleRate = sampleRate;
    mTrack.duration = duration;
    mTrack.frameRate = sampleRate / samplesPerFrame;
    mTrack.precision = 11;
    mTrack.regions.clear();

    // Create single region
    SMSRegion region;
    region.duration = duration;
    region.regionType = 0;

    // Analyze each frame
    for (int i = 0; i < totalFrames; i++) {
        int startSample = i * samplesPerFrame;

        SMSFrame frame;
        frame.index = i;
        // offset 320 in CSMSFrame is the frame's absolute time position, not duration
        frame.duration = (double)startSample / sampleRate;

        analyzeFrame(samples, startSample, windowSize, sampleRate, maxHarmonics, frame);

        region.frames.append(frame);
    }

    mTrack.regions.append(region);

    return true;
}

bool SmsGenerator::readWav(const QString& path, QVector<float>& samples, int& sampleRate,
                            double beginTime, double endTime)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        mError = "Cannot open WAV file";
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    // Read RIFF header
    char riff[4];
    file.read(riff, 4);
    if (strncmp(riff, "RIFF", 4) != 0) {
        mError = "Invalid WAV: missing RIFF";
        return false;
    }

    quint32 fileSize;
    stream >> fileSize;

    char wave[4];
    file.read(wave, 4);
    if (strncmp(wave, "WAVE", 4) != 0) {
        mError = "Invalid WAV: missing WAVE";
        return false;
    }

    // Find chunks
    int channels = 0;
    int bitsPerSample = 0;
    qint64 dataOffset = 0;
    quint32 dataSize = 0;

    while (!file.atEnd()) {
        char chunkId[4];
        quint32 chunkSize;
        file.read(chunkId, 4);
        stream >> chunkSize;

        if (strncmp(chunkId, "fmt ", 4) == 0) {
            quint16 audioFormat, numChannels;
            quint32 sr, byteRate;
            quint16 blockAlign, bits;

            stream >> audioFormat >> numChannels >> sr >> byteRate >> blockAlign >> bits;

            if (audioFormat != 1) {
                mError = "Only PCM WAV supported";
                return false;
            }

            sampleRate = sr;
            channels = numChannels;
            bitsPerSample = bits;

            if (chunkSize > 16) {
                file.seek(file.pos() + chunkSize - 16);
            }
        } else if (strncmp(chunkId, "data", 4) == 0) {
            dataOffset = file.pos();
            dataSize = chunkSize;
            break;
        } else {
            file.seek(file.pos() + chunkSize);
        }
    }

    if (dataSize == 0 || channels == 0) {
        mError = "Invalid WAV structure";
        return false;
    }

    // Calculate sample range
    int bytesPerSample = bitsPerSample / 8;
    int frameSize = channels * bytesPerSample;
    int totalSamples = dataSize / frameSize;

    int startSample = (int)(beginTime * sampleRate);
    int endSample = (endTime < 0) ? totalSamples : (int)(endTime * sampleRate);

    if (startSample >= totalSamples) startSample = 0;
    if (endSample > totalSamples) endSample = totalSamples;
    if (endSample <= startSample) endSample = totalSamples;

    int numSamples = endSample - startSample;
    samples.resize(numSamples);

    // Seek to start position
    file.seek(dataOffset + startSample * frameSize);

    // Read samples (convert to mono float)
    for (int i = 0; i < numSamples; i++) {
        float sum = 0.0f;
        for (int ch = 0; ch < channels; ch++) {
            if (bitsPerSample == 16) {
                qint16 sample;
                stream >> sample;
                sum += sample / 32768.0f;
            } else if (bitsPerSample == 8) {
                quint8 sample;
                stream >> sample;
                sum += (sample - 128) / 128.0f;
            } else if (bitsPerSample == 24) {
                quint8 b1, b2, b3;
                stream >> b1 >> b2 >> b3;
                qint32 sample = (b3 << 24) | (b2 << 16) | (b1 << 8);
                sample >>= 8; // Sign extend
                sum += sample / 8388608.0f;
            }
        }
        samples[i] = sum / channels;
    }

    return true;
}

void SmsGenerator::analyzeFrame(const QVector<float>& samples, int startSample, int windowSize,
                                 int sampleRate, int maxHarmonics, SMSFrame& frame)
{
    // Ensure we don't read past the end
    if (startSample + windowSize > samples.size()) {
        windowSize = samples.size() - startSample;
    }

    // Number of noise bands (Bark-scale, standard for SMS)
    const int noiseBands = 25;

    if (windowSize < 64) {
        frame.f0 = 100.0f; // Use default f0 for silent frames
        // Use natural harmonic count matching reference SMS format
        float nyquistSilent = sampleRate / 2.0f;
        int silentHarmonics = (int)ceil(nyquistSilent / frame.f0);
        silentHarmonics = qMin(silentHarmonics, maxHarmonics);
        for (int h = 0; h < silentHarmonics; h++) {
            SMSHarmonic harmonic;
            harmonic.frequency = frame.f0 * (h + 1);
            harmonic.amplitude = -100.0f;
            harmonic.phase = 0.0f;
            frame.harmonics.append(harmonic);
        }
        frame.noiseAmplitudes.fill(-100.0f, noiseBands);
        frame.noisePhases.fill(0.0f, noiseBands);
        return;
    }

    // Estimate fundamental frequency
    frame.f0 = estimateF0(samples, startSample, windowSize, sampleRate);

    if (frame.f0 < 20.0f || frame.f0 > 2000.0f) {
        frame.f0 = 100.0f; // Default fallback
    }

    // Compute DFT
    QVector<float> magnitudes, phases;
    computeDFT(samples, startSample, windowSize, magnitudes, phases);

    // Extract harmonics
    float binFreq = (float)sampleRate / windowSize;
    float nyquist = sampleRate / 2.0f;

    // Extract harmonics — match reference SMS format:
    // Natural harmonic count = ceil(nyquist / f0), typically 41-42
    // Frequencies stored as exact harmonic multiples: freq[i] = (i+1) * f0
    // (Engine rebuilds freq array in sub_10003530 anyway)
    int dftHarmonics = qMin(maxHarmonics, (int)(nyquist / frame.f0));

    for (int h = 1; h <= dftHarmonics; h++) {
        float targetFreq = frame.f0 * h;
        if (targetFreq > nyquist) break;

        int bin = (int)(targetFreq / binFreq + 0.5f);

        SMSHarmonic harmonic;
        // Store exact harmonic multiple (like reference SMS)
        harmonic.frequency = targetFreq;

        if (bin < magnitudes.size()) {
            // Find peak near expected bin
            int searchRange = 3;
            int bestBin = bin;
            float bestMag = magnitudes[bin];

            for (int b = qMax(1, bin - searchRange); b <= qMin(magnitudes.size() - 1, bin + searchRange); b++) {
                if (magnitudes[b] > bestMag) {
                    bestMag = magnitudes[b];
                    bestBin = b;
                }
            }

            // Convert magnitude to dB
            if (bestMag > 1e-10f) {
                harmonic.amplitude = 20.0f * log10f(bestMag);
            } else {
                harmonic.amplitude = -100.0f;
            }

            harmonic.phase = phases[bestBin];
        } else {
            harmonic.amplitude = -100.0f;
            harmonic.phase = 0.0f;
        }

        frame.harmonics.append(harmonic);
    }

    // Compute noise energy in bands between harmonics
    // Noise = spectral energy not accounted for by harmonics
    frame.noiseAmplitudes.resize(noiseBands);
    frame.noisePhases.resize(noiseBands);

    for (int band = 0; band < noiseBands; band++) {
        // Bark-scale-like frequency bands spanning 0 to nyquist
        float bandLow = nyquist * band / noiseBands;
        float bandHigh = nyquist * (band + 1) / noiseBands;

        int binLow = qMax(1, (int)(bandLow / binFreq));
        int binHigh = qMin(magnitudes.size() - 1, (int)(bandHigh / binFreq));

        // Compute average energy in this band, excluding harmonic peaks
        float totalEnergy = 0.0f;
        int count = 0;

        for (int b = binLow; b <= binHigh; b++) {
            // Check if this bin is near a harmonic
            bool isHarmonic = false;
            for (int h = 0; h < frame.harmonics.size(); h++) {
                int harmonicBin = (int)(frame.harmonics[h].frequency / binFreq + 0.5f);
                if (abs(b - harmonicBin) <= 2) {
                    isHarmonic = true;
                    break;
                }
            }
            if (!isHarmonic) {
                totalEnergy += magnitudes[b] * magnitudes[b];
                count++;
            }
        }

        if (count > 0 && totalEnergy > 1e-20f) {
            float rmsEnergy = sqrtf(totalEnergy / count);
            frame.noiseAmplitudes[band] = 20.0f * log10f(rmsEnergy);
        } else {
            frame.noiseAmplitudes[band] = -100.0f;
        }
        frame.noisePhases[band] = 0.0f;
    }
}

void SmsGenerator::computeDFT(const QVector<float>& samples, int startSample, int windowSize,
                               QVector<float>& magnitudes, QVector<float>& phases)
{
    int N = windowSize;
    int numBins = N / 2 + 1;

    magnitudes.resize(numBins);
    phases.resize(numBins);

    // Apply window and compute DFT
    for (int k = 0; k < numBins; k++) {
        float real = 0.0f;
        float imag = 0.0f;

        for (int n = 0; n < N; n++) {
            float sample = samples[startSample + n] * hannWindow(n, N);
            float angle = -2.0f * M_PI * k * n / N;
            real += sample * cosf(angle);
            imag += sample * sinf(angle);
        }

        magnitudes[k] = sqrtf(real * real + imag * imag) * 2.0f / N;
        phases[k] = atan2f(imag, real);
    }
}

float SmsGenerator::estimateF0(const QVector<float>& samples, int startSample, int windowSize, int sampleRate)
{
    // Autocorrelation-based pitch detection
    int minLag = sampleRate / 1000; // Max 1000 Hz
    int maxLag = sampleRate / 50;   // Min 50 Hz

    if (maxLag > windowSize / 2) maxLag = windowSize / 2;

    float bestCorr = 0.0f;
    int bestLag = minLag;

    for (int lag = minLag; lag < maxLag; lag++) {
        float corr = 0.0f;
        float norm1 = 0.0f;
        float norm2 = 0.0f;

        for (int i = 0; i < windowSize - lag; i++) {
            float s1 = samples[startSample + i];
            float s2 = samples[startSample + i + lag];
            corr += s1 * s2;
            norm1 += s1 * s1;
            norm2 += s2 * s2;
        }

        if (norm1 > 0 && norm2 > 0) {
            corr /= sqrtf(norm1 * norm2);
        }

        if (corr > bestCorr) {
            bestCorr = corr;
            bestLag = lag;
        }
    }

    if (bestCorr < 0.3f) {
        // Low correlation, probably noise
        return 100.0f;
    }

    return (float)sampleRate / bestLag;
}

float SmsGenerator::hannWindow(int n, int N)
{
    return 0.5f * (1.0f - cosf(2.0f * M_PI * n / (N - 1)));
}

float SmsGenerator::freqToCents(float freq)
{
    if (freq <= 0) return 0;
    // Cents relative to A4 (440 Hz)
    return 1200.0f * log2f(freq / 440.0f);
}

// Helper: write a double (8 bytes) in little-endian to QDataStream
// (QDataStream::SinglePrecision truncates doubles to 4 bytes, so we write raw)
static void writeDoubleLE(QDataStream& stream, double value)
{
    stream.writeRawData((const char*)&value, sizeof(double));
}

// Helper: write a chunk header placeholder, return position of size field
static qint64 writeChunkHeader(QFile& file, QDataStream& stream, const char* magic)
{
    stream.writeRawData(magic, 4);
    qint64 sizePos = file.pos();
    stream << (quint32)0; // placeholder for chunk size
    return sizePos;
}

// Helper: patch chunk size (size = total bytes from magic start to data end)
static void patchChunkSize(QFile& file, QDataStream& stream, qint64 sizeFieldPos)
{
    qint64 endPos = file.pos();
    // chunk size includes the 8-byte header (magic + size)
    quint32 chunkSize = (quint32)(endPos - (sizeFieldPos - 4));
    file.seek(sizeFieldPos);
    stream << chunkSize;
    file.seek(endPos);
}

bool SmsGenerator::writeSms(const QString& smsPath)
{
    QFile file(smsPath);
    if (!file.open(QIODevice::ReadWrite)) {
        mError = "Cannot create SMS file";
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    // SMS2 file format (based on DSE4 reverse engineering):
    //
    // CSMSCollection::Load calls CChunk::Read, so the file starts with
    // a CChunk header: [magic "SMS2" (4B)][total_size (4B)].
    // Then the specific reader (CSMSCollection::Read) reads:
    //   [metadata_buffer_size (4B)]
    //   [metadata (buffer_size bytes, omitted when 0)]
    //   [track_count (4B)]
    //   For each track: CChunk "GEN " ...
    //
    // Each CChunk has: [4-byte magic][4-byte size] header.
    // The size field = total bytes from magic start to data end (includes header).

    // 0. Outer CChunk header: "SMS2" + total size (patched later)
    qint64 sms2SizePos = writeChunkHeader(file, stream, "SMS2");

    // 1. Metadata buffer size = 0 (no index/metadata needed)
    stream << (quint32)0;

    // 2. Track count
    stream << (quint32)1;

    // 3. GEN chunk (CSMSGeneric)
    qint64 genSizePos = writeChunkHeader(file, stream, "GEN ");

    // Track type: 1 = single track
    stream << (quint8)1;

    // 4. GTRK chunk (CSMSGenericTrack)
    qint64 gtrkSizePos = writeChunkHeader(file, stream, "GTRK");

    // Track index
    stream << (quint32)mTrack.trackIndex;

    // Track flags: 0x0F = sampleRate|duration|frameRate|precision
    stream << (quint32)0x0F;

    // Sample rate (flags & 1)
    stream << (quint32)mTrack.sampleRate;

    // Duration as 8-byte double (flags & 2)
    writeDoubleLE(stream, mTrack.duration);

    // Frame rate (flags & 4)
    stream << (quint32)mTrack.frameRate;

    // Precision (flags & 8)
    stream << (quint8)mTrack.precision;

    // Region count
    stream << (quint32)mTrack.regions.size();

    // 5. Write each region
    for (const SMSRegion& region : mTrack.regions) {
        // RGN chunk (CSMSRegion)
        qint64 rgnSizePos = writeChunkHeader(file, stream, "RGN ");

        // Region duration (8-byte double) — NOT timeOffset!
        // sub_1006AE20 uses this field to compute track cumulative duration
        writeDoubleLE(stream, region.duration);

        // Region type (1 byte)
        stream << (quint8)region.regionType;

        // Flags1 (1 byte): no optional region fields
        stream << (quint8)0x00;

        // Flags2 (1 byte): no optional frame collections
        stream << (quint8)0x00;

        // Frame count
        stream << (quint32)region.frames.size();

        // 6. Write each frame (matching real VOCALOID growl format)
        float nyquist = mTrack.sampleRate / 2.0f;
        for (const SMSFrame& frame : region.frames) {
            // FRM2 chunk (CSMSFrame)
            qint64 frmSizePos = writeChunkHeader(file, stream, "FRM2");

            // Frame version/index (4 bytes) — real data uses 1
            stream << (quint32)1;

            // Time position (8-byte double)
            writeDoubleLE(stream, frame.duration);

            // Frame flags (64-bit): use real VOCALOID growl flags
            quint64 frameFlags = FRAME_FLAGS_GROWL;
            stream << frameFlags;

            // --- Harmonics (flags & 7) ---
            // Always write SMS_HARMONIC_SLOTS (350) slots, pad with zeros
            stream << (quint32)SMS_HARMONIC_SLOTS;

            // Frequencies (350 floats)
            for (int i = 0; i < SMS_HARMONIC_SLOTS; i++) {
                if (i < frame.harmonics.size())
                    stream << frame.harmonics[i].frequency;
                else
                    stream << (float)0.0f;
            }

            // Amplitudes (350 floats)
            for (int i = 0; i < SMS_HARMONIC_SLOTS; i++) {
                if (i < frame.harmonics.size())
                    stream << frame.harmonics[i].amplitude;
                else
                    stream << (float)-100.0f; // silence for unused slots
            }

            // Phases (350 floats)
            for (int i = 0; i < SMS_HARMONIC_SLOTS; i++) {
                if (i < frame.harmonics.size())
                    stream << frame.harmonics[i].phase;
                else
                    stream << (float)0.0f;
            }

            // --- No noise flags (0x10/0x20 not set) ---

            // --- F0 in Hz (flags & 0x200, NOT 0x80000000) ---
            // Engine converts Hz→cents internally in CSMSFrame::Read
            stream << frame.f0;

            // --- Growl parameters block (flags & 0x2000) ---
            writeGrowlBlock(stream, frame);

            // --- Marker byte after growl block ---
            stream << (quint8)0x00;

            // --- Extra byte (flags & 0x40000) ---
            stream << (quint8)0x00;

            // --- Extra float (flags & 0x80000) ---
            stream << (float)0.0f;

            // --- ENV1: Spectral envelope (flags & 0x80) ---
            writeSpectralEnvelope(stream, frame, nyquist, 0);

            // --- ENV3: Second spectral envelope (flags & 0x20000) ---
            // Write a sparser version with fewer control points
            writeSpectralEnvelope(stream, frame, nyquist, 38);

            // Patch FRM2 chunk size
            patchChunkSize(file, stream, frmSizePos);
        }

        // Patch RGN chunk size
        patchChunkSize(file, stream, rgnSizePos);
    }

    // Patch GTRK chunk size
    patchChunkSize(file, stream, gtrkSizePos);

    // Patch GEN chunk size
    patchChunkSize(file, stream, genSizePos);

    // Patch outer SMS2 chunk size
    patchChunkSize(file, stream, sms2SizePos);

    file.close();
    return true;
}

void SmsGenerator::writeGrowlBlock(QDataStream& stream, const SMSFrame& frame)
{
    // Growl parameters block read by sub_100674A0
    // First DWORD: sub-flags indicating which fields are present
    // Real data uses 0x001FFFFF (all 21 bits set)
    quint32 growlSubFlags = 0x001FFFFF;
    stream << growlSubFlags;

    // Default growl model parameters (from reference VY2V5 data)
    // bits 0-9: 10 individual float parameters
    stream << (float)0.066667f;   // bit 0: modulation rate
    stream << (float)0.011765f;   // bit 1
    stream << (float)0.004315f;   // bit 2
    stream << (float)1.000000f;   // bit 3
    stream << (float)0.132280f;   // bit 4
    stream << (float)-23.6499f;   // bit 5
    stream << (float)0.031790f;   // bit 6
    stream << (float)-0.015801f;  // bit 7
    stream << (float)0.000000f;   // bit 8
    stream << (float)-1.10224f;   // bit 9

    // bit 10: 160 bytes (40 floats) — noise/residual array 1
    for (int i = 0; i < 40; i++)
        stream << (float)0.0f;

    // bit 11: 160 bytes (40 floats) — noise/residual array 2
    for (int i = 0; i < 40; i++)
        stream << (float)0.0f;

    // bit 12: 80 bytes (20 floats) — additional array 1
    for (int i = 0; i < 20; i++)
        stream << (float)0.0f;

    // bit 13: 80 bytes (20 floats) — additional array 2
    for (int i = 0; i < 20; i++)
        stream << (float)0.0f;

    // bits 14-19: 6 individual float parameters
    stream << (float)2.93317f;    // bit 14
    stream << (float)0.005755f;   // bit 15
    stream << (float)-4.01324f;   // bit 16
    stream << (float)3.15770f;    // bit 17
    stream << (float)4.29826f;    // bit 18
    stream << (float)0.903109f;   // bit 19
}

void SmsGenerator::writeSpectralEnvelope(QDataStream& stream,
                                          const SMSFrame& frame, float nyquist,
                                          int maxPoints)
{
    // Build control points: (normalized_freq_position, dB_amplitude)
    // from the harmonic data
    QVector<QPair<float, float>> points;

    // Add point at position 0
    if (!frame.harmonics.isEmpty()) {
        points.append({0.0f, frame.harmonics[0].amplitude});
    } else {
        points.append({0.0f, -100.0f});
    }

    // One point per non-zero harmonic
    for (int i = 0; i < frame.harmonics.size(); i++) {
        float normPos = frame.harmonics[i].frequency / nyquist;
        if (normPos > 0.0f && normPos <= 1.0f) {
            points.append({normPos, frame.harmonics[i].amplitude});
        }
    }

    // If maxPoints > 0, downsample to that many points
    if (maxPoints > 0 && points.size() > maxPoints) {
        QVector<QPair<float, float>> sparse;
        for (int i = 0; i < maxPoints; i++) {
            int idx = i * (points.size() - 1) / (maxPoints - 1);
            sparse.append(points[idx]);
        }
        points = sparse;
    }

    // Ensure at least 2 points
    if (points.size() < 2) {
        points.append({1.0f, -100.0f});
    }

    // ENV chunk format:
    // CChunk header: [magic "ENV " 4B][chunk_size 4B]
    // Specific data: [env_flags 4B][data_length 4B][param1 4B][param2 4B][param3 4B][data]
    // Data: pairs of (position_float, dB_float) = 8 bytes per point

    quint32 dataLength = points.size() * 8; // pairs of floats
    quint32 chunkSize = 8 + 4 + 4 + 4 + 4 + 4 + dataLength;

    stream << SMS_MAGIC_ENV;
    stream << chunkSize;

    // ENV flags: 0x01820002 (from real data: type 3/spline with specific encoding)
    stream << (quint32)0x01820002;

    // Data length
    stream << dataLength;

    // Params: range min, range max, default
    stream << (float)-20000.0f;
    stream << (float)20000.0f;
    stream << (float)0.0f;

    // Control points: pairs of (dB_value, normalized_position)
    // Note: real data order is (position, dB) based on the pair analysis
    for (const auto& pt : points) {
        stream << pt.first;   // normalized frequency position
        stream << pt.second;  // amplitude in dB
    }
}

bool SmsGenerator::writeVqmIni(const QString& iniPath, const QString& sampleName,
                                double begin, double end, double pitch, const QString& wavFilename)
{
    QFile file(iniPath);

    // Read existing content if file exists
    QString existingContent;
    if (file.exists()) {
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            existingContent = in.readAll();
            file.close();
        }
    }

    // Check if section already exists
    QString sectionHeader = QString("[%1]").arg(sampleName);
    if (existingContent.contains(sectionHeader)) {
        // Remove existing section
        QStringList lines = existingContent.split('\n');
        QStringList newLines;
        bool inSection = false;

        for (const QString& line : lines) {
            if (line.trimmed().startsWith('[')) {
                inSection = (line.trimmed() == sectionHeader);
            }
            if (!inSection) {
                newLines.append(line);
            }
        }
        existingContent = newLines.join('\n');
    }

    // Append new section
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    if (!existingContent.isEmpty()) {
        out << existingContent;
        if (!existingContent.endsWith('\n')) {
            out << "\n";
        }
    }

    out << QString("[%1]\n").arg(sampleName);
    out << QString("begin=%1\n").arg(begin, 0, 'f', 6);
    out << QString("end=%1\n").arg(end, 0, 'f', 6);
    out << QString("pitch=%1\n").arg(pitch, 0, 'f', 1);
    out << QString("file=%1\n").arg(wavFilename);

    file.close();
    return true;
}

bool SmsGenerator::copyWav(const QString& srcPath, const QString& dstPath)
{
    // Ensure destination directory exists
    QFileInfo dstInfo(dstPath);
    QDir().mkpath(dstInfo.absolutePath());

    // Remove existing file if present
    if (QFile::exists(dstPath)) {
        QFile::remove(dstPath);
    }

    return QFile::copy(srcPath, dstPath);
}
