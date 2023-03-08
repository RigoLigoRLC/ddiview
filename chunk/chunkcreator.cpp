#include "chunkcreator.h"

#include "chunkarray.h"
#include "dbsinger.h"
#include "dbvoice.h"
#include "emptychunk.h"
#include "dbvarticulation.h"
#include "dbvarticulationphu.h"
#include "dbvarticulationphupart.h"
#include "dbvstationary.h"
#include "dbvstationaryphu.h"
#include "dbvstationaryphupart.h"
#include "dbvvqmorph.h"
#include "dbvvqmorphphu.h"
#include "dbvvqmorphphupart.h"
#include "dbtimbre.h"
#include "dbtimbremodel.h"

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
    AddToFactory<ChunkDBVoice>();
    AddToFactory<ChunkEmptyChunk>();
    AddToFactory<ChunkDBVArticulation>();
    AddToFactory<ChunkDBVArticulationPhU>();
    AddToFactory<ChunkDBVArticulationPhUPart>();
    AddToFactory<ChunkDBVStationary>();
    AddToFactory<ChunkDBVStationaryPhU>();
    AddToFactory<ChunkDBVStationaryPhUPart>();
    AddToFactory<ChunkDBVVQMorph>();
    AddToFactory<ChunkDBVVQMorphPhU>();
    AddToFactory<ChunkDBVVQMorphPhUPart>();
    AddToFactory<ChunkDBTimbre>();
    AddToFactory<ChunkDBTimbreModel>();

    for(auto i : FactoryMethods.keys())
        qDebug() << i << FactoryMethods[i];

    mProgressDlg = nullptr;
}

ChunkCreator *ChunkCreator::Get()
{
    if (mInstance == nullptr)
        mInstance = new ChunkCreator;
    return mInstance;
}

BaseChunk *ChunkCreator::ReadFor(QByteArray signature, FILE *file)
{
    if(mProgressDlg) mProgressDlg->setValue(ftell(file));
    auto pos = ftell(file); if(pos == 0x30a5a) __debugbreak();
    if(!FactoryMethods.contains(signature))
        return nullptr;
    auto ret = FactoryMethods[signature]();
    ret->Read(file);
    return ret;
}

template<typename T>
void ChunkCreator::AddToFactory()
{
    FactoryMethods[T::DefaultSignature()] = &T::Make;
}
