#ifndef DBVARTICULATIONPHUPART_DEVDB_H
#define DBVARTICULATIONPHUPART_DEVDB_H

#include "chunk/chunkreaderguards.h"
#include "chunk/smsgenerictrack.h"
#include "chunkarray.h"
#include "item_directory.h"

class ChunkDBVArticulationPhUPart_DevDB : public ChunkChunkArray {
public:
    explicit ChunkDBVArticulationPhUPart_DevDB() : ChunkChunkArray() {

    }

    static QByteArray ClassSignature() { return "ARTp"; }
    virtual QByteArray ObjectSignature() { return ClassSignature(); }

    virtual void Read(FILE *file) {
        uint32_t frameCount = 0;
        allFramesCount = 0;
        skipFrameCount = SIZE_MAX;
//        ItemDirectory* sectionDir = nullptr;
        ReadBlockSignature(file);
        ReadArrayHead(file);
        CHUNK_TREADPROP("unk1", 8, PropHex64);
        CHUNK_READPROP("unk2", 2);
        CHUNK_TREADPROP("mPitch", 4, PropF32);
        CHUNK_TREADPROP("Average pitch", 4, PropF32);
        CHUNK_READPROP("unk5", 4);
        CHUNK_TREADPROP("Dynamic", 4, PropF32);
        CHUNK_TREADPROP("Tempo", 4, PropF32);

        {
            ArrayLeadingNameGuard lng(true);
            ReadArrayBody(file, 0);
        }

        // Calculate frame count on the fly
        ChunkSMSGenericTrackChunk* epr;
        size_t tempSkipFrameCount = SIZE_MAX;
        if ((epr = decltype(epr)(GetChildByName("EpR")))) {
            for (auto i : epr->Children) {
                auto rgn = (ChunkSMSRegionChunk*)i;
                if (rgn->GetPropertiesMap().contains("Stable region begin")) {
                    frameCount += rgn->Children.size();
                    framesToWrite.append(rgn->Children);
                    if (skipFrameCount == SIZE_MAX) {
                        // Do it only once
                        skipFrameCount = tempSkipFrameCount;
                    }
                } else if (skipFrameCount == SIZE_MAX) {
                    // Accumulate before met first non skipped region
                    tempSkipFrameCount += rgn->Children.size();
                }
                allFramesCount += rgn->Children.size();
            }
        }
        mAdditionalProperties["Frame count"] = {QByteArray((const char*)&frameCount, 4), PropU32Int, SIZE_MAX};
        this->frameCount = frameCount;


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
//        CHUNK_TREADPROP("SND Sample count", 4, PropU32Int);
//        CHUNK_TREADPROP("SND Sample offset", 8, PropHex64);
//        CHUNK_TREADPROP("SND Sample offset+800", 8, PropHex64);
//        CHUNK_TREADPROP("Guessed_Section count", 4, PropU32Int);
//        STUFF_INTO(GetProperty("Guessed_Section count").data, sectionCount, uint32_t);
//        if(sectionCount) {
//            sectionDir = new ItemDirectory;
//            sectionDir->SetName("<sections>");
//            Children.append(sectionDir);
//        }
//        for(int i = 0; i < sectionCount; i++) {
//            auto artSec = new ItemArticulationSection;
//            artSec->Read(file);
//            artSec->SetName(QString("<section %1>").arg(i));
//            sectionDir->Children.append(artSec);
//        }
//        ReadStringName(file);
    }

    bool IsStartingWithVowel() {
        // Only vowels are considered stretchable
        // so if stable mark of section 0 is present it must be starting with a vowel
        auto rgn = GetChildByName("EpR")->Children[1]; // region 0 is leading cutoff
        return rgn->GetProperty("Stable region begin").data != QByteArray(4, '\0');
    }

    virtual QString Description() {
        return "DBVArticulationPhUPart";
    }

    static BaseChunk* Make() { return new ChunkDBVArticulationPhUPart_DevDB; }

    size_t frameCount, allFramesCount, skipFrameCount;
    QVector<BaseChunk*> framesToWrite;
};

#endif // DBVARTICULATIONPHUPART_DEVDB_H
