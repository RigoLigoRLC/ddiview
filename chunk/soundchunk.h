#ifndef SOUNDCHUNK_H
#define SOUNDCHUNK_H

#include <QFile>
#include <QDebug>
#include "basechunk.h"
#include "propertytype.h"
#include "common.h"
#include "util/util.h"

class ChunkSoundChunk : public BaseChunk {
public:
    explicit ChunkSoundChunk() : BaseChunk() {

    }

    static QByteArray ClassSignature() { return "SND "; }
    virtual QByteArray ObjectSignature() { return ClassSignature(); }

    virtual void Read(FILE *file) {
        ReadBlockSignature(file);
        CHUNK_TREADPROP("Sample rate", 4, PropU32Int);
        CHUNK_TREADPROP("Channel count", 2, PropU16Int);
        CHUNK_TREADPROP("Sample count", 4, PropU32Int);

        STUFF_INTO(GetProperty("Sample count").data, sampleCount, uint32_t);

        // Read sample data directly
        sampleData.resize(sampleCount * 2); // 16bit samples
        fread(sampleData.data(), 1, sampleCount * 2, file);
    }

    virtual QString Description() {
        return "Sound chunk";
    }

    // [from, to) samples
    // Returns a complete SND chunk with header: "SND " + size(4) + sampleRate(4) + channelCount(2) + sampleCount(4) + data
    QByteArray GetTruncatedChunk(int64_t from, int64_t to) {
        // Clamp to valid range
        if (from < 0) from = 0;
        if (to > (int64_t)sampleCount) to = sampleCount;
        if (from >= to) {
            qWarning() << "GetTruncatedChunk: invalid range from:" << from << "to:" << to << "sampleCount:" << sampleCount;
            from = 0;
            to = sampleCount;
        }

        uint32_t truncatedSampleCount = to - from;

        // Build chunk: signature(4) + chunkSize(4) + sampleRate(4) + channelCount(2) + sampleCount(4) + data
        QByteArray chunk;
        chunk.append("SND ", 4);                                    // signature
        uint32_t chunkSize = 4 + 2 + 4 + truncatedSampleCount * 2;  // size excludes signature and size field
        chunk.append((const char*)&chunkSize, 4);                   // chunk size
        chunk.append(GetProperty("Sample rate").data);              // 4 bytes
        chunk.append(GetProperty("Channel count").data);            // 2 bytes
        chunk.append((const char*)&truncatedSampleCount, 4);        // 4 bytes
        chunk.append(sampleData.mid(from * 2, truncatedSampleCount * 2));  // sample data

        // Verify size
        int expectedTotalSize = 4 + 4 + 4 + 2 + 4 + truncatedSampleCount * 2;
        if (chunk.size() != expectedTotalSize) {
            qWarning() << "GetTruncatedChunk: size mismatch! expected:" << expectedTotalSize
                       << "actual:" << chunk.size()
                       << "from:" << from << "to:" << to
                       << "truncatedSampleCount:" << truncatedSampleCount
                       << "sampleCount:" << sampleCount
                       << "sampleData.size:" << sampleData.size();
        }

        return chunk;
    }

    int16_t* begin() { return (int16_t*)(sampleData.data()); }
    int16_t* end() { return (int16_t*)(sampleData.data() + sampleData.length()); }

    static BaseChunk* Make() { return new ChunkSoundChunk; }

    QByteArray sampleData;
    uint32_t sampleCount;
};

#endif // SOUNDCHUNK_H
