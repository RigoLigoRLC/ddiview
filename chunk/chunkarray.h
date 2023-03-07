#ifndef CHUNKARRAY_H
#define CHUNKARRAY_H

#include "basechunk.h"
#include "chunkcreator.h"

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

    void ReadArrayBody(FILE* file) {
        char signatureBuf[4];
        uint32_t count;

        // Read subchunk count
        CHUNK_READPROP("Count", 4);
        STUFF_INTO(GetProperty("Count"), count, uint32_t);

        // Read signature first
        fpos_t pos;
        fgetpos(file, &pos);
        // Skip Leading QWORD if needed
        if(HasLeadingQword) fseek(file, 8, SEEK_CUR);
        fread(signatureBuf, 1, 4, file);
        fsetpos(file, &pos);

        //FIXME: JUST FOR TEST
        ChunkCreator::Get()->ReadFor(signatureBuf, file);
    }
};

#endif // CHUNKARRAY_H
