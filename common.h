#ifndef COMMON_H
#define COMMON_H

#include <QString>

namespace Common {
    QString RelativePitchToNoteName(float pitch);
    int RelativePitchToMidiNote(float pitch);
    QString MidiNoteToNoteName(int note);
}

#endif // COMMON_H
