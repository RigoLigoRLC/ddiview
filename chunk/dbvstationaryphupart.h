#ifndef DBVSTATIONARYPHUPART_H
#define DBVSTATIONARYPHUPART_H

#include "chunkarray.h"

class ChunkDBVStationaryPhUPart : public ChunkChunkArray {
public:
    explicit ChunkDBVStationaryPhUPart() : ChunkChunkArray() {

    }

    static QByteArray DefaultSignature() { return "STAp"; }

    virtual void Read(FILE *file) {
        uint32_t dataCount, unk15;
        ReadBlockSignature(file);
        ReadArrayHead(file);
        CHUNK_TREADPROP("unk1", 8, PropHex64);
        CHUNK_READPROP("unk2", 2);
        CHUNK_TREADPROP("mPitch", 4, PropF32);
        CHUNK_TREADPROP("Average pitch", 4, PropF32);
        CHUNK_READPROP("unk5", 4);
        CHUNK_TREADPROP("Dynamic", 4, PropF32);
        CHUNK_TREADPROP("Tempo", 4, PropF32);
        CHUNK_READPROP("unk8", 4);
        ReadArrayBody(file, 0);
        CHUNK_READPROP("unk9", 4);
        CHUNK_TREADPROP("Data count", 4, PropU32Int);
        STUFF_INTO(GetProperty("Data count").data, dataCount, uint32_t);
        CHUNK_READPROP("Data", 8 * dataCount);
        CHUNK_TREADPROP("SND Sample rate", 4, PropU32Int);
        CHUNK_TREADPROP("SND Channel count", 2, PropU16Int);
        CHUNK_READPROP("unk12", 4);
        CHUNK_TREADPROP("SND DDB offset", 8, PropHex64);
        CHUNK_READPROP("unk14", 4);
        CHUNK_READPROP("unk15", 4);
        CHUNK_READPROP("unk16", 4);
        CHUNK_READPROP("unk17", 4);
        ReadStringName(file);
    }

    virtual QString Description() {
        return "DBVStationaryPhUPart";
    }

    static BaseChunk* Make() { return new ChunkDBVStationaryPhUPart; }
};

#endif // DBVSTATIONARYPHUPART_H
