#ifndef SKIPCHUNK_H
#define SKIPCHUNK_H

#include "basechunk.h"
#include "util/util.h"

class ChunkSkipChunk : public BaseChunk {
public:
    explicit ChunkSkipChunk() : BaseChunk() {

    }

    static QByteArray ClassSignature() { return "____Skipped"; }
    virtual QByteArray ObjectSignature() { return ClassSignature(); }

    virtual void Read(FILE *file) {
        ReadBlockSignature(file);
        mName = QString("<Skipped %1>").arg(QString(mSignature));

        myfseek64(file, mSize - 8, SEEK_CUR);
    }

    virtual QString Description() {
        return "<Skipped chunk>";
    }

    static BaseChunk* Make() { return new ChunkSkipChunk; }
};

#endif // SKIPCHUNK_H
