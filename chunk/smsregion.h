#ifndef SMSREGION_H
#define SMSREGION_H

#include "basechunk.h"
#include "smsframe.h"
#include "skipchunk.h"

class ChunkSMSRegionChunk : public BaseChunk {
public:
    explicit ChunkSMSRegionChunk() : BaseChunk() {

    }

    static QByteArray ClassSignature() { return "RGN "; }
    virtual QByteArray ObjectSignature() { return ClassSignature(); }

    virtual void Read(FILE *file) {
        ReadBlockSignature(file);

        CHUNK_TREADPROP("TimeOffset", 8, PropF64);         // time offset
        CHUNK_TREADPROP("RegionType", 1, PropU8Int);       // region type

        uint8_t flags1, flags2;
        CHUNK_TREADPROP("Flags1", 1, PropU8Int); STUFF_INTO(GetProperty("Flags1").data, flags1, uint8_t);
        if (flags1 & 0x01) {
            CHUNK_TREADPROP("ExtFlags", 4, PropU32Int);           // extended flags
            CHUNK_TREADPROP("AttackTime", 4, PropF32);            // attack time
            CHUNK_TREADPROP("ReleaseTime", 4, PropF32);           // release time
            CHUNK_TREADPROP("SustainLevel", 4, PropF32);          // sustain level
            CHUNK_TREADPROP("DecayTime", 4, PropF32);             // decay time
            CHUNK_TREADPROP("PeakLevel", 4, PropF32);             // peak level
            CHUNK_TREADPROP("InitialLevel", 4, PropF32);          // initial level
            CHUNK_TREADPROP("FinalLevel", 4, PropF32);            // final level
            CHUNK_TREADPROP("VibratoDepth", 4, PropF32);          // vibrato depth
            CHUNK_TREADPROP("VibratoRate", 8, PropF64);           // vibrato rate
            CHUNK_TREADPROP("VibratoDelay", 8, PropF64);          // vibrato delay
            CHUNK_READPROP("PitchBendData", 80);                  // pitch bend data
            CHUNK_READPROP("DynamicsData", 160);                  // dynamics data
            CHUNK_READPROP("ExpressionData", 28);                 // expression data
        }
        if (flags1 & 0x02)
            CHUNK_TREADPROP("VoiceType", 1, PropU8Int);           // voice type
        if (flags1 & 0x04)
            CHUNK_TREADPROP("ScoringNoteIndex", 4, PropU32Int);   // scoring note index
        if (flags1 & 0x08) {
            uint32_t segCount;
            CHUNK_TREADPROP("SegmentCount", 4, PropU32Int); STUFF_INTO(GetProperty("SegmentCount").data, segCount, uint32_t);
            CHUNK_READPROP("SegmentData", segCount * 16);         // segment data
        }
        if (flags1 & 0x10) {
            uint32_t pitchCount;
            CHUNK_TREADPROP("PitchPointCount", 4, PropU32Int); STUFF_INTO(GetProperty("PitchPointCount").data, pitchCount, uint32_t);
            CHUNK_READPROP("PitchContour", pitchCount * 8);       // pitch contour
        }
        if (flags1 & 0x20) {
            CHUNK_READPROP("TimbreParams", 24);                   // timbre parameters
            CHUNK_TREADPROP("StableBegin", 4, PropU32Int);        // stable region begin
            CHUNK_TREADPROP("StableEnd", 4, PropU32Int);          // stable region end
        }
        if (flags1 & 0x40) {
            CHUNK_READPROP("ExtraParams", 48);                    // extra parameters
        }

        CHUNK_TREADPROP("Flags2", 1, PropU8Int); STUFF_INTO(GetProperty("Flags2").data, flags2, uint8_t);
        if (flags2 & 0x01) CHUNK_READCHILD(ChunkSkipChunk, this); // Envelope 1
        if (flags2 & 0x02) CHUNK_READCHILD(ChunkSkipChunk, this); // Envelope 2
        if (flags2 & 0x04) CHUNK_READCHILD(ChunkSkipChunk, this); // Envelope 3
        if (flags2 & 0x08) CHUNK_READCHILD(ChunkSkipChunk, this); // Envelope 4
        if (flags2 & 0x10) CHUNK_READCHILD(ChunkSkipChunk, this); // Envelope 5
        if (flags2 & 0x20) CHUNK_READCHILD(ChunkSkipChunk, this); // ThinEnvelope
        if (flags2 & 0x40) {
            CHUNK_TREADPROP("Stable region begin", 4, PropU32Int);
            CHUNK_TREADPROP("Stable region end", 4, PropU32Int);
        }

        uint32_t frameCount;
        CHUNK_TREADPROP("Frame count", 4, PropU32Int); STUFF_INTO(GetProperty("Frame count").data, frameCount, uint32_t);
        for (size_t ii = 0; ii < frameCount; ii++) {
            auto frame = new ChunkSMSFrameChunk;
            frame->Read(file);
            frame->SetName(
                QString("Frame %1")
                    .arg(static_cast<qulonglong>(ii), 5, 10, QChar('0'))
            );

            Children.append(frame);
        }
    }

    virtual QString Description() {
        return "SMSRegion";
    }

    static BaseChunk* Make() { return new ChunkSMSRegionChunk; }
};

#endif // SMSREGION_H
