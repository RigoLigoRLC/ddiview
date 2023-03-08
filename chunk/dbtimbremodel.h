#ifndef DBVTIMBREMODEL_H
#define DBVTIMBREMODEL_H

#include "chunkarray.h"

class ChunkDBTimbreModel : public ChunkChunkArray {
public:
    explicit ChunkDBTimbreModel() : ChunkChunkArray() {

    }

    static QByteArray DefaultSignature() { return "TMM "; }

    virtual void Read(FILE *file) {
        ReadBlockSignature(file);
        ReadArrayHead(file);
        CHUNK_READPROP("unk1", 4);
        ReadArrayBody(file, 0);
        ReadStringName(file);
    }

    virtual QString Description() {
        return "DBTimbreModel";
    }

    static BaseChunk* Make() { return new ChunkDBTimbreModel; }
};

#endif // DBVTIMBREMODEL_H
