#include "ddi.h"
#include "chunk/chunkcreator.h"
#include <stdio.h>
#include <QMessageBox>

BaseChunk* Parse(QString path)
{
    FILE *file = fopen(path.toLocal8Bit(), "rb");
    if(!file) {
        QMessageBox::critical(nullptr, "error opening file", "DDI parser failed to open file");
        return false;
    }

    BaseChunk* chunk = ChunkCreator::Get()->ReadFor("DBSe", file);

    fclose(file);

    return chunk;
}
