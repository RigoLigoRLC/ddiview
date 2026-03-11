#ifndef ITEM_PHONETICUNIT_H
#define ITEM_PHONETICUNIT_H

#include "basechunk.h"

class ItemPhoneticUnit : public BaseChunk {
public:
    explicit ItemPhoneticUnit() : BaseChunk() {

    }

    static QByteArray ClassSignature() { return "____PhoneticUnit"; }
    virtual QByteArray ObjectSignature() { return ClassSignature(); }

    virtual void Read(FILE *file) {
        BaseChunk::Read(file);
        QByteArray tmp(18, 0);
        fread(tmp.data(), 1, 18, file);
        // Trim at first null byte
        int nullPos = tmp.indexOf('\0');
        if (nullPos >= 0) {
            tmp.truncate(nullPos);
        }
        mName = tmp;

        CHUNK_TREADPROP("PhonemeIndex", 4, PropU32Int);     // phoneme index
        CHUNK_TREADPROP("HasEpREnvelope", 4, PropU32Int);   // has EpR envelope
        CHUNK_TREADPROP("HasResEnvelope", 4, PropU32Int);   // has resonance envelope
        CHUNK_TREADPROP("Unvoiced", 1, PropU8Int);          // is unvoiced
    }

    virtual QString Description() {
        return "<Phonetic unit>";
    }

    static BaseChunk* Make() { return new ItemPhoneticUnit; }
};

#endif // ITEM_PHONETICUNIT_H
