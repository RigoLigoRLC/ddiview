#ifndef SOUNDCHUNK_H
#define SOUNDCHUNK_H

#define FP_TYPE double

#include <QFile>
#include <fstream>
#include <QDebug>
#include "basechunk.h"
#include "propertytype.h"
#include "common.h"
#include "ciglet.h"

class ChunkSoundChunk : public BaseChunk {
public:
    explicit ChunkSoundChunk() : BaseChunk() {

    }

    static QByteArray ClassSignature() { return "SND "; }
    virtual QByteArray ObjectSignature() { return ClassSignature(); }

    virtual void Read(FILE *file) {
        auto originalOffset = ftell(file);

        ReadBlockSignature(file);
        CHUNK_TREADPROP("Sample rate", 4, PropU32Int);
        CHUNK_TREADPROP("Channel count", 2, PropU16Int);
        CHUNK_TREADPROP("Sample count", 4, PropU32Int);

        STUFF_INTO(GetProperty("Sample count").data, sampleCount, uint32_t);
        //CHUNK_TREADPROP("Samples", sampleCount, PropRawHex);

        auto samplesOffset = ftell(file);

        QFile snd;
        if (snd.open(file, QFile::ReadOnly)) {
            snd.seek(samplesOffset);
            sampleData = snd.read(sampleCount * 2); // 16bit samples
            fseek(file, snd.pos(), SEEK_SET); // not sure if needed
            snd.close();
        }

    }

    virtual QString Description() {
        return "Sound chunk";
    }

    // [from, to) samples
    QByteArray GetTruncatedChunk(uint32_t from, uint32_t to) {
        QByteArray ret("SND \0\0\0\0", 8);
        uint32_t sampleCount = to - from;
        ret.append(GetProperty("Sample rate").data);
        ret.append(GetProperty("Channel count").data);
        ret.append((const char*)&sampleCount, 4);
        ret.append(sampleData.mid(from * 2, sampleCount * 2));
        uint32_t size = ret.size();
        return QByteArray("SND ", 4) + QByteArray((const char*)&size, 4) + ret.mid(8);
    }

    int16_t* begin() { return (int16_t*)(sampleData.data()); }
    int16_t* end() { return (int16_t*)(sampleData.data() + sampleData.length()); }

    // Optimized "from" point
    uint32_t OptimizedFrom(uint32_t from, QByteArray relativePitchData) {
        float relPitch; STUFF_INTO(relativePitchData, relPitch, 4);
        float freq = Common::RelativePitchToFrequency(relPitch);
        int periodSamps = round(44100.0 / freq);

        // Go back one period
        uint32_t bfrom = max(0, (int)from - periodSamps);
//        uint32_t backLength = from - bfrom;

        // Take two periods of samples and convolve with a square window with length periodSamps/2
        int windowLen = periodSamps / 2;
        auto window = zeros(1, windowLen);
        for (size_t i = 0; i < windowLen; i++) window[i] = 1.0;

        int dataLen = periodSamps * 2;
        auto data = zeros(1, dataLen);
        auto _begin = begin();
        for (size_t i = 0; i < dataLen; i++) data[i] = (double)(_begin[bfrom + i]);

        auto convResult = conv(data, window, dataLen, windowLen);
//        std::ofstream ofs("D:/tmp/convResult.csv");
//        for (size_t i = 0; i < dataLen + windowLen - 1; i++) {
//            ofs << convResult[i] << ",\n";
//        }
//        ofs.close();

//        std::ofstream ofs2("D:/tmp/data.csv");
//        for (size_t i = 0; i < dataLen; i++) {
//            ofs2 << data[i] << ",\n";
//        }
//        ofs2.close();


        // Find zero crossing points in convolution results
        auto zc = Common::FindZeroCrossing(convResult, convResult + dataLen + windowLen - 1);
        // Find closest zero crossing point relative to center
        auto center = (dataLen + windowLen - 1 ) / 2;
        int distance = INT_MAX, closest = 0;

        foreach (auto i, zc) {
            auto dist = abs(long(i - center));
            if (dist < distance) {
                distance = dist;
                closest = i;
            }
        }
        free(convResult);
        free(data);
        free(window);

        if (zc.isEmpty()) {
            qDebug() << "Empty ZC, cannot optimize";
            return from;
        }


        closest += bfrom + periodSamps / 4;
        qDebug() << "From" << from << "Optimize" << closest;

        return closest;
    }

    static BaseChunk* Make() { return new ChunkSoundChunk; }

    QByteArray sampleData;
    uint32_t sampleCount;
};

#endif // SOUNDCHUNK_H
