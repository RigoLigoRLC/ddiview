#include "chunkcreator.h"

#include "chunkarray.h"
#include "dbsinger.h"

#include <QDebug>

ChunkCreator *ChunkCreator::mInstance = nullptr;
bool BaseChunk::HasLeadingQword = true;
const int BaseChunk::ItemChunkRole = Qt::UserRole + 1;

ChunkCreator::ChunkCreator(QObject *parent)
    : QObject{parent}
{
    if (!FactoryMethods.empty())
        return;

    AddToFactory<ChunkChunkArray>();
    AddToFactory<ChunkDBSinger>();

    for(auto i : FactoryMethods.keys())
        qDebug() << i << FactoryMethods[i];
}

ChunkCreator *ChunkCreator::Get()
{
    if (mInstance == nullptr)
        mInstance = new ChunkCreator;
    return mInstance;
}

BaseChunk *ChunkCreator::ReadFor(QByteArray signature, FILE *file)
{
    auto ret = FactoryMethods[signature]();
    ret->Read(file);
    return ret;
}

template<typename T>
void ChunkCreator::AddToFactory()
{
    FactoryMethods[T::DefaultSignature()] = &T::Make;
}
