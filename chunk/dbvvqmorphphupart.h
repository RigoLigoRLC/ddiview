#ifndef DBVVQMORPHPHUPART_H
#define DBVVQMORPHPHUPART_H

#include "chunkarray.h"

class ChunkDBVVQMorphPhUPart : public ChunkChunkArray {
public:
    explicit ChunkDBVVQMorphPhUPart() : ChunkChunkArray() {

    }

    static QByteArray ClassSignature() { return "VQMp"; }
    virtual QByteArray ObjectSignature() { return ClassSignature(); }

    virtual void Read(FILE *file) {
        uint32_t dataCount;
        ReadBlockSignature(file);
        ReadArrayHead(file);
        CHUNK_TREADPROP("TimeInfo", 8, PropHex64);         // time info
        CHUNK_TREADPROP("Flags", 2, PropU16Int);           // flags
        CHUNK_TREADPROP("mPitch", 4, PropF32);             // relative pitch
        CHUNK_TREADPROP("Average pitch", 4, PropF32);      // average pitch
        CHUNK_TREADPROP("PitchDeviation", 4, PropF32);     // pitch deviation
        CHUNK_TREADPROP("Dynamic", 4, PropF32);            // dynamics/velocity
        CHUNK_TREADPROP("Tempo", 4, PropF32);              // tempo
        ReadArrayBody(file, 0);
        CHUNK_TREADPROP("FrameDataSize", 4, PropU32Int);   // frame data size
        CHUNK_TREADPROP("Frame count", 4, PropU32Int);
        STUFF_INTO(GetProperty("Frame count").data, dataCount, uint32_t);
        CHUNK_READPROP("FrameRefs", 8 * dataCount);        // frame references array
        CHUNK_TREADPROP("SND Sample rate", 4, PropU32Int);
        CHUNK_TREADPROP("SND Channel count", 2, PropU16Int);
        CHUNK_TREADPROP("SND Sample count", 4, PropU32Int);
        CHUNK_TREADPROP("SND Sample offset", 8, PropHex64);
        CHUNK_TREADPROP("EpRTrackIndex", 4, PropS32Int);   // EpR track index (-1=none)
        CHUNK_TREADPROP("ResTrackIndex", 4, PropS32Int);   // residual track index (-1=none)
        CHUNK_TREADPROP("OptionalIndex3", 4, PropS32Int);  // optional index 3 (-1=none)
        CHUNK_TREADPROP("OptionalIndex4", 4, PropS32Int);  // optional index 4 (-1=none)
        ReadStringName(file);
    }

    virtual QString Description() {
        return "DBVVQMorphPhUPart";
    }

    static BaseChunk* Make() { return new ChunkDBVVQMorphPhUPart; }
};

#endif // DBVARTICULATIONPHUPART_H
