#ifndef SMSFRAME_H
#define SMSFRAME_H

#include "basechunk.h"
#include "util/util.h"
#include <QByteArray>
#include <QFile>

class ChunkSMSFrameChunk : public BaseChunk {
public:
    explicit ChunkSMSFrameChunk() : BaseChunk() {

    }

    static QByteArray ClassSignature() { return "FRM2"; }
    virtual QByteArray ObjectSignature() { return ClassSignature(); }

    virtual void Read(FILE *file) {
        auto originalOffset = myftell64(file);

        ReadBlockSignature(file);

        // Read the entire frame data for later writing
        QFile f;
        if (f.open(file, QFile::ReadOnly)) {
            f.seek(originalOffset);
            rawData = f.read(mSize);
            myfseek64(file, originalOffset + mSize, SEEK_SET);
            f.close();
        }
    }

    virtual QString Description() {
        return "SMSFrame";
    }

    QByteArray rawData;

    static BaseChunk* Make() { return new ChunkSMSFrameChunk; }
};

#endif // SMSFRAME_H
