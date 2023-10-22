#ifndef DBVARTICULATIONPHU_DEVDB_H
#define DBVARTICULATIONPHU_DEVDB_H

#include "chunk/dbvarticulationphupart_devdb.h"
#include "chunkarray.h"

class ChunkDBVArticulationPhU_DevDB : public ChunkChunkArray {
public:
    explicit ChunkDBVArticulationPhU_DevDB() : ChunkChunkArray() {

    }

    static QByteArray ClassSignature() { return "ARTu"; }
    virtual QByteArray ObjectSignature() { return ClassSignature(); }

    virtual void Read(FILE *file) {
        ReadBlockSignature(file);
        ReadArrayHead(file);
        CHUNK_READPROP("unk1", 4);
        CHUNK_READPROP("unk2", 4);
        CHUNK_READPROP("unk3", 4);
        CHUNK_READPROP("unk4", 4);
        CHUNK_READPROP("unk5", 4);

//        ReadArrayBody(file, 0);
        // Read subchunk count
        char signatureBuf[4];
        uint32_t count;
        CHUNK_TREADPROP("Count", 4, PropU32Int);
        STUFF_INTO(GetProperty("Count").data, count, uint32_t);
        for(uint32_t ii = 0; ii < count; ii++) {
            ReadStringName(file); // HACK: Use current array's name as a temporary variable

            // Read signature first
            fpos_t pos;
            fgetpos(file, &pos);
            // Skip Leading QWORD if needed
            if(HasLeadingQword) fseek(file, 8, SEEK_CUR);
            fread(signatureBuf, 1, 4, file);
            fsetpos(file, &pos);

            auto sig = QByteArray(signatureBuf, 4);
            auto chk = new ChunkDBVArticulationPhUPart_DevDB;
            chk->Read(file);

            chk->SetName(GetName());

            Children.append(chk);
        }
    }

    virtual QString Description() {
        return "DBVArticulationPhU";
    }

    static BaseChunk* Make() { return new ChunkDBVArticulationPhU_DevDB; }
};

#endif // DBVARTICULATIONPHU_DEVDB_H
