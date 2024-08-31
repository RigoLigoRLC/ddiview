#ifndef SMSFRAME_H
#define SMSFRAME_H

#include "basechunk.h"
#include <QByteArray>
#include <QFile>

class ChunkSMSFrameChunk : public BaseChunk {
public:
    explicit ChunkSMSFrameChunk() : BaseChunk() {

    }

    static QByteArray ClassSignature() { return "FRM2"; }
    virtual QByteArray ObjectSignature() { return ClassSignature(); }

    virtual void Read(FILE *file) {
        // auto originalOffset = ftell(file);

        ReadBlockSignature(file);

        // QFile snd;
        // if (snd.open(file, QFile::ReadOnly)) {
        //     snd.seek(originalOffset);
        //     rawData = snd.read(mSize);
        //     fseek(file, snd.pos(), SEEK_SET);
        //     snd.close();
        // }

        fseek(file, mSize - 8, SEEK_CUR);
    }

    virtual QString Description() {
        return "SMSFrame";
    }

    QByteArray rawData;

    static BaseChunk* Make() { return new ChunkSMSFrameChunk; }
};

#endif // SMSFRAME_H
