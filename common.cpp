#include "common.h"
#include <cmath>

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
    static const char *NoteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    if (note > 108 || note < 21)
        return QString();
    note -= 12;
    return QString("%1%2").arg(NoteNames[note % 12]).arg(note / 12);
}

QString Common::SanitizeFilename(QString filename)
{
    QString ret = filename;
    static QString illegalChars = "\\/:%*?\"<>|;=";
    for(auto i = ret.begin(); i != ret.end(); i++)
        if(illegalChars.indexOf(*i) != -1)
            *i = '_';
    return ret;
}

QString Common::EscapeStringForCsv(QString str)
{
    QString ret;
    for(auto i = str.begin(); i != str.end(); i++)
        if(*i == ',' || *i == '"' || *i == '\\')
            ret += QString('\\') + *i;
        else
            ret += *i;
    return ret;
}
