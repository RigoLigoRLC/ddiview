#include "common.h"


QString Common::RelativePitchToNoteName(float pitch)
{
    return MidiNoteToNoteName(RelativePitchToMidiNote(pitch));
}

int Common::RelativePitchToMidiNote(float pitch)
{
    // relative pitch = cents deviated from A4 (440Hz, MIDI 69)
    float relativeSemitone = pitch / 100.0;
    relativeSemitone = roundf(relativeSemitone);
    return 69 + relativeSemitone;
}

QString Common::MidiNoteToNoteName(int note)
{
    // A0 = MIDI 21, C8 = 108. Return empty when out of range
    static const char * NoteNames[] = {
        "A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"
    };
    if(note > 108 || note < 21) return QString();
    note -= 21;
    return QString("%1%2").arg(NoteNames[note % 12]).arg(note / 12);
}
