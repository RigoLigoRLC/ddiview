#include "chunk/chunkreaderguards.h"
#ifndef DBVSTATIONARYPHUPART_DEVDB_H
#define DBVSTATIONARYPHUPART_DEVDB_H

#include "chunkarray.h"
#include "smsgenerictrack.h"
#include "smsregion.h"

class ChunkDBVStationaryPhUPart_DevDB : public ChunkChunkArray {
public:
    explicit ChunkDBVStationaryPhUPart_DevDB() : ChunkChunkArray() {

    }

    static QByteArray ClassSignature() { return "STAp"; }
    virtual QByteArray ObjectSignature() { return ClassSignature(); }

    virtual void Read(FILE *file) {
        uint32_t frameCount = 0;
        ReadBlockSignature(file);
        ReadArrayHead(file);
        CHUNK_TREADPROP("unk1", 8, PropHex64);
        CHUNK_READPROP("unk2", 2);
        CHUNK_TREADPROP("mPitch", 4, PropF32);
        CHUNK_TREADPROP("Average pitch", 4, PropF32);
        CHUNK_READPROP("unk5", 4);
        CHUNK_TREADPROP("Dynamic", 4, PropF32);
        CHUNK_TREADPROP("Tempo", 4, PropF32);
        CHUNK_READPROP("unk8", 4);
        {
            ArrayLeadingNameGuard lng(true);
            ReadArrayBody(file, 0);
        }

        // Calculate frame count on the fly
        ChunkSMSGenericTrackChunk* epr;
        if ((epr = decltype(epr)(GetChildByName("EpR")))) {
            for (auto i : epr->Children) {
                auto rgn = (ChunkSMSRegionChunk*)i;
                if (rgn->GetPropertiesMap().contains("Stable region begin")) {
                    frameCount += rgn->Children.size();
                }
            }
        }
        mAdditionalProperties["Frame count"] = {QByteArray((const char*)&frameCount, 4), PropU32Int, SIZE_MAX};

//        CHUNK_READPROP("unk9", 4);

//        CHUNK_TREADPROP("Frame count", 4, PropU32Int);
//        STUFF_INTO(GetProperty("Frame count").data, frameCount, uint32_t);
//        {
//            auto frameDir = new ItemAudioFrameRefs(frameCount);
//            frameDir->SetName("<Frames>");
//            frameDir->Read(file);
//            Children.append(frameDir);
//        }

//        CHUNK_TREADPROP("SND Sample rate", 4, PropU32Int);
//        CHUNK_TREADPROP("SND Channel count", 2, PropU16Int);
//        CHUNK_READPROP("SND Sample count", 4);
//        CHUNK_TREADPROP("SND Sample offset", 8, PropHex64);
//        CHUNK_READPROP("unk14", 4);
//        CHUNK_READPROP("unk15", 4);
//        CHUNK_READPROP("unk16", 4);
//        CHUNK_READPROP("unk17", 4);
//        ReadStringName(file);
    }

    virtual QString Description() {
        return "DBVStationaryPhUPart";
    }

    static BaseChunk* Make() { return new ChunkDBVStationaryPhUPart_DevDB; }
};

#endif // DBVSTATIONARYPHUPART_DEVDB_H
