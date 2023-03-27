#include "uicommon.h"
#include <QGuiApplication>
#include <QScreen>

int UiCommon::SystemUnitDpi()
{
#ifdef Q_OS_MACOS
    return 72;
#else
    return 96;
#endif
}

double UiCommon::SystemScalingFactor()
{
    return qApp->primaryScreen()->logicalDotsPerInch() / SystemUnitDpi();
}
