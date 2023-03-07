#ifndef PHONEMEDICT_H
#define PHONEMEDICT_H

#include "basechunk.h"
#include "phonemegroup.h"
#include "item_phoneticunit.h"
#include "item_directory.h"
#include "item_eprguides.h"

class ChunkPhonemeDict : public BaseChunk {
public:
    explicit ChunkPhonemeDict() : BaseChunk() {

    }

    static QByteArray DefaultSignature() { return "PHDC"; }

    virtual void Read(FILE* file) {
        ReadBlockSignature(file);
        CHUNK_READPROP("Flags", 4);
        CHUNK_READPROP("Phoneme count", 4);

        // Phoneme directory
        auto PhonemeDir = new ItemDirectory;
        Children.append(PhonemeDir);
        uint32_t PhonemeCount;
        STUFF_INTO(GetProperty("Phoneme count"), PhonemeCount, uint32_t);
        for(uint32_t ii = 0; ii < PhonemeCount; ii++) {
            CHUNK_READCHILD(ItemPhoneticUnit, PhonemeDir);
        }

        // Detect flag
        uint32_t flags; STUFF_INTO(GetProperty("Flags"), flags, uint32_t);

        if(flags & 0x04) { // Phonetic group
            CHUNK_READCHILD(ChunkPhonemeGroup, this);
        }

        // EpR Guides
        CHUNK_READCHILD(ItemEprGuidesGroup, this);
    }

    virtual QString Description() {
        return "PhonemeDictionary";
    }

    static BaseChunk* Make() {
        return new ChunkPhonemeDict();
    }
};

#endif // PHONEMEDICT_H
