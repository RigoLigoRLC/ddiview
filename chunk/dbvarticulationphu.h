#ifndef DBVARTICULATIONPHU_H
#define DBVARTICULATIONPHU_H

#include "chunkarray.h"

class ChunkDBVArticulationPhU : public ChunkChunkArray {
public:
    explicit ChunkDBVArticulationPhU() : ChunkChunkArray() {

    }

    static QByteArray ClassSignature() { return "ARTu"; }
    virtual QByteArray ObjectSignature() { return ClassSignature(); }

    virtual void Read(FILE *file) {
        ReadBlockSignature(file);
        ReadArrayHead(file);
        CHUNK_TREADPROP("Index", 4, PropU32Int);           // phoneme index
        CHUNK_TREADPROP("TargetIndex1", 4, PropU32Int);    // target index 1
        CHUNK_TREADPROP("TargetIndex2", 4, PropU32Int);    // target index 2
        CHUNK_TREADPROP("TargetIndex3", 4, PropU32Int);    // target index 3
        CHUNK_TREADPROP("TargetIndex4", 4, PropU32Int);    // target index 4
        ReadArrayBody(file, 0);
        ReadStringName(file);
    }

    virtual QString Description() {
        return "DBVArticulationPhU";
    }

    static BaseChunk* Make() { return new ChunkDBVArticulationPhU; }
};

#endif // DBVARTICULATIONPHU_H
