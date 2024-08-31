#ifndef UTIL_H
#define UTIL_H

#include <QString>

static inline int64_t myftell64(FILE* a) {
#ifdef __CYGWIN__
    return ftell(a);
#elif defined (_WIN32)
    return _ftelli64(a);
#else
    return ftello(a);
#endif
}

static inline int myfseek64(FILE* a, int64_t offset, int origin) {
#ifdef __CYGWIN__
    return fseek(a, offset, origin);
#elif defined (_WIN32)
    return _fseeki64(a, offset, origin);
#else
    return fseeko(a, offset, origin);
#endif
}

#endif // UTIL_H
