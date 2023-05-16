#ifndef ITEM_AUDIOFRAMEREFS_H
#define ITEM_AUDIOFRAMEREFS_H

#include "basechunk.h"

class ItemAudioFrameRefs : public BaseChunk {
public:
    explicit ItemAudioFrameRefs(uint32_t count) : BaseChunk(), m_count(count) {
        mAdditionalProperties["Count"] = ChunkProperty {
                QByteArray::fromRawData((const char *)&m_count,
                                        sizeof(m_count)), PropU32Int
        };
    }

    static QByteArray ClassSignature() { return "____AudioFrameRefs"; }
    virtual QByteArray ObjectSignature() { return ClassSignature(); }

    virtual void Read(FILE *file) {
        for (uint32_t i = 0; i < m_count; i++) {
            CHUNK_TREADPROP(QString("Frame %1").arg(i, 5, 10, QChar('0')), 8, PropHex64);
        }
    }

    virtual QString Description() {
        return "<Audio frame references>";
    }

    static BaseChunk* Make() { return new ItemAudioFrameRefs(0); }

protected:
    uint32_t m_count = 0;
};

#endif // ITEM_AUDIOFRAMEREFS_H
