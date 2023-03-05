#ifndef BASECHUNK_H
#define BASECHUNK_H

#include <QObject>
#include <QByteArray>
#include <QMap>
#include <stdio.h>

#define CHUNK_READPROP(name,size)            \
    do {                                     \
        QByteArray tmp(size, 0);          \
        fread(tmp.data(), 1, size, file);      \
        mAdditionalProperties[name] = tmp;   \
    } while (0)

#define STUFF_INTO(from,to,type) \
    memcpy(&to, from.constData(), sizeof(type))

#define CHUNK_READCHILD(type,whose) \
    do { \
        type* child = new type; \
        child->Read(file); \
        whose->Children.append(child); \
    } while(0)

class BaseChunk
{
public:
    explicit BaseChunk() {
        mSignature = QByteArray(4, 0);
        mSize = 0;
    }

    explicit BaseChunk(BaseChunk& that) {
        this->mName = that.mName;
        this->mSize = that.mSize;
        this->mSignature = that.mSignature;
    }

    static bool HasLeadingQword;
    const static int ItemChunkRole;

    static QByteArray DefaultSignature() { return "    "; }
    virtual void Read(FILE* file) {}
    virtual QString Description() { return "..."; }
    static BaseChunk* Make() { return nullptr; }

    QString GetName() { return mName; }
    void SetName(QString name) { mName = name; }
    QByteArray GetProperty(QString name) { return mAdditionalProperties.value(name, QByteArray()); }
    void SetProperty(QString name, QByteArray data) { mAdditionalProperties[name] = data; }
    const QMap<QString, QByteArray>& GetPropertiesMap() { return mAdditionalProperties; }

    QVector<BaseChunk*> Children;

protected:
    QByteArray mSignature;
    uint32_t mSize;
    QString mName;

    QMap<QString, QByteArray> mAdditionalProperties;

    void ReadBlockSignature(FILE* file) {
        QByteArray tmp(4, 0);
        fread(tmp.data(), 1, 4, file); mSignature = tmp;
        fread(tmp.data(), 1, 4, file); STUFF_INTO(tmp, mSize, uint32_t);
    }

};

Q_DECLARE_METATYPE(BaseChunk*);

#endif // BASECHUNK_H
