#ifndef SOUNDCHUNK_H
#define SOUNDCHUNK_H

#include <QFile>
#include "basechunk.h"
#include "propertytype.h"

class ChunkSoundChunk : public BaseChunk {
public:
    explicit ChunkSoundChunk() : BaseChunk() {

    }

    static QByteArray ClassSignature() { return "SND "; }
    virtual QByteArray ObjectSignature() { return ClassSignature(); }

    virtual void Read(FILE *file) {
        auto originalOffset = ftell(file);

        ReadBlockSignature(file);
        CHUNK_TREADPROP("Sample rate", 4, PropU32Int);
        CHUNK_TREADPROP("Channel count", 2, PropU16Int);
        CHUNK_TREADPROP("Sample count", 4, PropU32Int);

        uint32_t sampleCount;
        STUFF_INTO(GetProperty("Sample count").data, sampleCount, uint32_t);
        //CHUNK_TREADPROP("Samples", sampleCount, PropRawHex);

        QFile snd;
        if (snd.open(file, QFile::ReadOnly)) {
            snd.seek(originalOffset);
            rawData = snd.read(mSize);
            fseek(file, snd.pos(), SEEK_SET);
            snd.close();
        }

    }

    virtual QString Description() {
        return "Sound chunk";
    }

    static BaseChunk* Make() { return new ChunkSoundChunk; }

    QByteArray rawData;
};

#endif // SOUNDCHUNK_H
