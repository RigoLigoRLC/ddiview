#include "ddi.h"
#include "chunk/chunkcreator.h"
#include <stdio.h>
#include <QMessageBox>

BaseChunk* ParseDdi(QString path)
{
    FILE *file = fopen(path.toLocal8Bit(), "rb");
    if(!file) {
        QMessageBox::critical(nullptr, "error opening file", "DDI parser failed to open file");
        return nullptr;
    }

    // Detect Development DB tree file
    BaseChunk::DevDb = path.endsWith(".tree");

    // Read signature first
    fpos_t pos;
    char signatureBuf[4];
    fgetpos(file, &pos);
    // Skip Leading QWORD if needed
    if(BaseChunk::HasLeadingQword) fseek(file, 8, SEEK_CUR);
    fread(signatureBuf, 1, 4, file);
    fsetpos(file, &pos);

    auto sig = QByteArray(signatureBuf, 4);
    auto chunk = ChunkCreator::Get()->ReadFor("DBSe", file);

    fclose(file);

    return chunk;
}
