#ifndef SMSGENERICTRACK_H
#define SMSGENERICTRACK_H

#include "basechunk.h"
#include "smsregion.h"

class ChunkSMSGenericTrackChunk : public BaseChunk {
public:
    explicit ChunkSMSGenericTrackChunk() : BaseChunk() {

    }

    static QByteArray ClassSignature() { return "GTRK"; }
    virtual QByteArray ObjectSignature() { return ClassSignature(); }

    virtual void Read(FILE *file) {
        ReadBlockSignature(file);

        CHUNK_TREADPROP("TrackType", 4, PropU32Int);       // track type
        CHUNK_TREADPROP("Flags", 4, PropU32Int);           // flags
        CHUNK_TREADPROP("SampleRate", 4, PropU32Int);      // sample rate
        CHUNK_TREADPROP("Duration", 8, PropF64);           // duration
        CHUNK_TREADPROP("FrameRate", 4, PropU32Int);       // frame rate
        CHUNK_TREADPROP("Precision", 1, PropU8Int);        // precision
        CHUNK_TREADPROP("Region count", 4, PropU32Int);

        uint32_t rgnCount; STUFF_INTO(GetProperty("Region count").data, rgnCount, uint32_t);
        for (size_t ii = 0; ii < rgnCount; ii++) {
            auto rgn = new ChunkSMSRegionChunk;
            rgn->Read(file);
            rgn->SetName(QString("Region %1").arg(ii));
            Children.append(rgn);
        }
    }

    virtual QString Description() {
        return "SMSGenericTrack";
    }

    static BaseChunk* Make() { return new ChunkSMSGenericTrackChunk; }
};

#endif // SMSGENERICTRACK_H
