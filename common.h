#ifndef COMMON_H
#define COMMON_H

#include <QString>
#include <QList>

#ifdef Q_OS_UNIX
#include <signal.h>
#endif

namespace Common {
    QString RelativePitchToNoteName(float pitch);
    int RelativePitchToMidiNote(float pitch);
    float RelativePitchToFrequency(float pitch);
    QString MidiNoteToNoteName(int note);
    QString SanitizeFilename(QString filename);
    QString EscapeStringForCsv(QString str);
    QString devDbDirEncode(QString input);

    template <typename T> QList<size_t> FindZeroCrossing(T* begin, T* end) {
        QList<size_t> ret;
        bool sign = *begin < 0;
        auto at = begin;
        while (++at != end) {
            bool currSign = (*at < 0);
            if (currSign != sign) {
                ret.append(at - begin);
                sign = currSign;
            }
        }
        return ret;
    }
}

#endif // COMMON_H
