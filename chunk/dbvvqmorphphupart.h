#ifndef DBVVQMORPHPHUPART_H
#define DBVVQMORPHPHUPART_H

#include "chunkarray.h"

class ChunkDBVVQMorphPhUPart : public ChunkChunkArray {
public:
    explicit ChunkDBVVQMorphPhUPart() : ChunkChunkArray() {

    }

    static QByteArray DefaultSignature() { return "VQMp"; }

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
        CHUNK_READPROP("unk9", 4);
        CHUNK_READPROP("Data count", 4);
        STUFF_INTO(GetProperty("Data count"), dataCount, uint32_t);
        CHUNK_READPROP("Data", 8 * dataCount);
        CHUNK_READPROP("SND Sample rate", 4);
        CHUNK_READPROP("SND Channel count", 2);
        CHUNK_READPROP("unk12", 4);
        CHUNK_READPROP("SND DDB offset", 8);
        CHUNK_READPROP("unk14", 4);
        CHUNK_READPROP("unk15", 4);
        CHUNK_READPROP("unk16", 4);
        CHUNK_READPROP("unk17", 4);
        ReadStringName(file);
    }

    virtual QString Description() {
        return "DBVVQMorphPhUPart";
    }

    static BaseChunk* Make() { return new ChunkDBVVQMorphPhUPart; }
};

#endif // DBVARTICULATIONPHUPART_H
