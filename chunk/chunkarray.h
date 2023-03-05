#ifndef CHUNKARRAY_H
#define CHUNKARRAY_H

#include "basechunk.h"

class ChunkChunkArray : public BaseChunk {
public:
    explicit ChunkChunkArray() {

    }

    static QByteArray DefaultSignature() { return "ARR "; }

    virtual void Read(FILE* file) {
        if(HasLeadingQword)
            CHUNK_READPROP("LeadingQword", 8);
        ReadBlockSignature(file);
        ReadArrayHead(file);
    }

    virtual QString Description() {
        return "ChunkArray";
    }

    static BaseChunk* Make() {
        return new ChunkChunkArray();
    }

protected:
    void ReadArrayHead(FILE* file) {
        CHUNK_READPROP("unk4", 4);
        CHUNK_READPROP("UseEmptyChunk", 4);
    }
};

#endif // CHUNKARRAY_H
