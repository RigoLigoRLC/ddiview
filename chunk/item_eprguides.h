#ifndef ITEM_EPRGUIDES_H
#define ITEM_EPRGUIDES_H

#include "basechunk.h"

class ItemEprGuide : public BaseChunk {
public:
    explicit ItemEprGuide() : BaseChunk() {

    }

    static QByteArray DefaultSignature() { return "____EprGuide"; }

    virtual void Read(FILE *file) {
        uint32_t paramCount;
        CHUNK_READPROP("Name", 32);
        CHUNK_READPROP("Parameter count", 4);
        STUFF_INTO(GetProperty("Parameter count"), paramCount, uint32_t);
        for(uint32_t i = 0; i < paramCount; i++) {
            CHUNK_READPROP(QString("Offset %1 A").arg(i, 4, 0), 8);
            CHUNK_READPROP(QString("Offset %1 B").arg(i, 4, 0), 8);
        }
        SetName(GetProperty("Name"));
    }

    virtual QString Description() {
        return "(EpR Guide)";
    }

    static BaseChunk* Make() { return new ItemEprGuide; }
};

class ItemEprGuidesGroup : public BaseChunk {
public:
    explicit ItemEprGuidesGroup() : BaseChunk() {

    }

    static QByteArray DefaultSignature() { return "____EprGuides"; }

    virtual void Read(FILE *file) {
        uint32_t groupCount;
        CHUNK_READPROP("Count", 4);
        STUFF_INTO(GetProperty("Count"), groupCount, uint32_t);
        for(uint32_t i = 0; i < groupCount; i++) {
            CHUNK_READCHILD(ItemEprGuide, this);
        }
    }

    virtual QString Description() {
        return "(EpR Guides Group)";
    }

    static BaseChunk* Make() { return new ItemEprGuidesGroup; }
};

#endif // ITEM_EPRGUIDES_H
