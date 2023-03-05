#ifndef DBSINGER_H
#define DBSINGER_H

#include "chunkarray.h"
#include "phonemedict.h"

class ChunkDBSinger : public ChunkChunkArray {
public:
    explicit ChunkDBSinger() : ChunkChunkArray() {

    }

    static QByteArray DefaultSignature() { return "DBSe"; }

    virtual void Read(FILE* file) {
        ChunkChunkArray::Read(file);
        CHUNK_READPROP("unk6", 4);
        CHUNK_READCHILD(ChunkPhonemeDict, this);
    }

    virtual QString Description() {
        return "DBSinger";
    }

    static BaseChunk* Make() {
        return new ChunkDBSinger();
    }
};

#endif // DBSINGER_H
