#ifndef ITEMDIRECTORY_H
#define ITEMDIRECTORY_H

#include "basechunk.h"

class ItemDirectory : public BaseChunk {
public:
    explicit ItemDirectory() : BaseChunk() {

    }

    static QByteArray DefaultSignature() { return ""; }

    virtual void Read(FILE *file) {

    }

    virtual QString Description() {
        return "(Directory)";
    }

    static BaseChunk* Make() { return new ItemDirectory; }
};

#endif // ITEMDIRECTORY_H
