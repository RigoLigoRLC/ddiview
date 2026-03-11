#ifndef DBVARTICULATIONPHUPART_H
#define DBVARTICULATIONPHUPART_H

#include "chunkarray.h"
#include "item_directory.h"
#include "item_audioframerefs.h"
#include "item_articulationsection.h"

class ChunkDBVArticulationPhUPart : public ChunkChunkArray {
public:
    explicit ChunkDBVArticulationPhUPart() : ChunkChunkArray() {

    }

    static QByteArray ClassSignature() { return "ARTp"; }
    virtual QByteArray ObjectSignature() { return ClassSignature(); }

    virtual void Read(FILE *file) {
        uint32_t frameCount, sectionCount;
        ItemDirectory* sectionDir = nullptr;
        ReadBlockSignature(file);
        ReadArrayHead(file);
        CHUNK_TREADPROP("TimeInfo", 8, PropHex64);         // time info
        CHUNK_TREADPROP("Flags", 2, PropU16Int);           // flags
        CHUNK_TREADPROP("mPitch", 4, PropF32);             // relative pitch
        CHUNK_TREADPROP("Average pitch", 4, PropF32);      // average pitch
        CHUNK_TREADPROP("PitchDeviation", 4, PropF32);     // pitch deviation
        CHUNK_TREADPROP("Dynamic", 4, PropF32);            // dynamics/velocity
        CHUNK_TREADPROP("Tempo", 4, PropF32);              // tempo
        ReadArrayBody(file, 0);

        CHUNK_TREADPROP("Frame count", 4, PropU32Int);
        STUFF_INTO(GetProperty("Frame count").data, frameCount, uint32_t);
        {
            auto frameDir = new ItemAudioFrameRefs(frameCount);
            frameDir->SetName("<Frames>");
            frameDir->Read(file);
            Children.append(frameDir);
        }

        CHUNK_TREADPROP("SND Sample rate", 4, PropU32Int);
        CHUNK_TREADPROP("SND Channel count", 2, PropU16Int);
        CHUNK_TREADPROP("SND Sample count", 4, PropU32Int);
        CHUNK_TREADPROP("SND Sample offset", 8, PropHex64);
        CHUNK_TREADPROP("SND Sample offset+800", 8, PropHex64);
        CHUNK_TREADPROP("Section count", 4, PropU32Int);   // half-phone section count
        STUFF_INTO(GetProperty("Section count").data, sectionCount, uint32_t);
        if(sectionCount) {
            sectionDir = new ItemDirectory;
            sectionDir->SetName("<sections>");
            Children.append(sectionDir);
        }
        for(int i = 0; i < sectionCount; i++) {
            auto artSec = new ItemArticulationSection;
            artSec->Read(file);
            artSec->SetName(QString("<section %1>").arg(i));
            sectionDir->Children.append(artSec);
        }
        ReadStringName(file);
    }

    virtual QString Description() {
        return "DBVArticulationPhUPart";
    }

    static BaseChunk* Make() { return new ChunkDBVArticulationPhUPart; }
};

#endif // DBVARTICULATIONPHUPART_H
