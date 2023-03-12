#ifndef DBVARTICULATIONPHUPART_H
#define DBVARTICULATIONPHUPART_H

#include "chunkarray.h"

class ChunkDBVArticulationPhUPart : public ChunkChunkArray {
public:
    explicit ChunkDBVArticulationPhUPart() : ChunkChunkArray() {

    }

    static QByteArray DefaultSignature() { return "ARTp"; }

    virtual void Read(FILE *file) {
        uint32_t dataCount, unk15;
        ReadBlockSignature(file);
        ReadArrayHead(file);
        CHUNK_READPROP("unk1", 8);
        CHUNK_READPROP("unk2", 2);
        CHUNK_READPROP("unk3", 4);
        CHUNK_READPROP("unk4", 4);
        CHUNK_READPROP("unk5", 4);
        CHUNK_READPROP("unk6", 4);
        CHUNK_READPROP("unk7", 4);
        ReadArrayBody(file, 0);
        CHUNK_READPROP("Data count", 4);
        STUFF_INTO(GetProperty("Data count").data, dataCount, uint32_t);
        CHUNK_READPROP("Data", 8 * dataCount);
        CHUNK_READPROP("SND Sample rate", 4);
        CHUNK_READPROP("SND Channel count", 2);
        CHUNK_READPROP("unk12", 4);
        CHUNK_READPROP("SND DDB offset", 8);
        CHUNK_READPROP("SND DDB offset+800", 8);
        CHUNK_READPROP("unk15", 4);
        STUFF_INTO(GetProperty("unk15").data, unk15, uint32_t);
        CHUNK_READPROP("unk16", 4 * unk15 * 4); // 4 * unk15 * u32
        ReadStringName(file);
    }

    virtual QString Description() {
        return "DBVArticulationPhUPart";
    }

    static BaseChunk* Make() { return new ChunkDBVArticulationPhUPart; }
};

#endif // DBVARTICULATIONPHUPART_H
