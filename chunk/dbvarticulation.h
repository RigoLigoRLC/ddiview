#ifndef DBVARTICULATION_H
#define DBVARTICULATION_H

#include "chunkarray.h"

class ChunkDBVArticulation : public ChunkChunkArray {
public:
    explicit ChunkDBVArticulation() : ChunkChunkArray() {

    }

    static QByteArray DefaultSignature() { return "ART "; }

    virtual void Read(FILE *file) {
        ReadBlockSignature(file);
        ReadArrayHead(file);
        CHUNK_READPROP("unk1", 4);
        ReadArrayBody(file, 0);
        ReadStringName(file);
    }

    virtual QString Description() {
        return "DBVArticulation";
    }

    static BaseChunk* Make() { return new ChunkDBVArticulation; }
};

#endif // DBVARTICULATION_H
