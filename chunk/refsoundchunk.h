#ifndef REFSOUNDCHUNK_H
#define REFSOUNDCHUNK_H

#include "basechunk.h"

class ChunkRefSoundChunk : public BaseChunk {
public:
    explicit ChunkRefSoundChunk() : BaseChunk() {

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

        sampleOffset = ftell(file);
        sampleBytes = mSize - 0x12;
        fseek(file, sampleBytes, SEEK_CUR); // Skip sample data
    }

    virtual QString Description() {
        return "Sound chunk (Ref)";
    }

    static BaseChunk* Make() { return new ChunkRefSoundChunk; }

    uint32_t sampleCount;
    size_t sampleOffset, sampleBytes;
};

#endif // REFSOUNDCHUNK_H
