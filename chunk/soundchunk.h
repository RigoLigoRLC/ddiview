#ifndef SOUNDCHUNK_H
#define SOUNDCHUNK_H

#define FP_TYPE double

#include <QFile>
#include <fstream>
#include <QDebug>
#include "basechunk.h"
#include "propertytype.h"
#include "common.h"
// #include "ciglet.h"

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
        Q_UNUSED(relativePitchData);
        return from;
    }

    static BaseChunk* Make() { return new ChunkSoundChunk; }

    QByteArray sampleData;
    uint32_t sampleCount;
};

#endif // SOUNDCHUNK_H
