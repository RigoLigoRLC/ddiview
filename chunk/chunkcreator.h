#ifndef CHUNKCREATOR_H
#define CHUNKCREATOR_H

#include <QObject>
#include <QMap>
#include "basechunk.h"

typedef BaseChunk*(*MakeMethod)() ;

class ChunkCreator : public QObject
{
    Q_OBJECT
public:
    explicit ChunkCreator(QObject *parent = nullptr);
    static ChunkCreator* Get();

    BaseChunk *ReadFor(QByteArray signature, FILE* file);

private:
    static ChunkCreator* mInstance;

    QMap<QByteArray, MakeMethod> FactoryMethods;
    template <typename T> void AddToFactory();

signals:

};

#endif // CHUNKCREATOR_H
