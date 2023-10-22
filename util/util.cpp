
#include "util.h"

QString devDbDirEncode(QString input)
{
    QString ret;
    for (QChar c : input) {
        if (c < 'a' || c > 'z') {
            ret += '%' + QString::number(c.toLatin1()) + '%';
        } else {
            ret += c;
        }
    }
    return ret;
}
