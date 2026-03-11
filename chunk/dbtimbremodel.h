#ifndef DBVTIMBREMODEL_H
#define DBVTIMBREMODEL_H

#include "chunkarray.h"

class ChunkDBTimbreModel : public ChunkChunkArray {
public:
    explicit ChunkDBTimbreModel() : ChunkChunkArray() {

    }

    static QByteArray ClassSignature() { return "TMM "; }
    virtual QByteArray ObjectSignature() { return ClassSignature(); }

    virtual void Read(FILE *file) {
        ReadBlockSignature(file);
        ReadArrayHead(file);
        CHUNK_TREADPROP("ModelIndex", 4, PropU32Int);  // model index
        ReadArrayBody(file, 0);
        ReadStringName(file);
    }

    virtual QString Description() {
        return "DBTimbreModel";
    }

    static BaseChunk* Make() { return new ChunkDBTimbreModel; }
};

#endif // DBVTIMBREMODEL_H
