#include "global.h"

#include <QString>
#include <QObject>

QString Global::getIconSVGPath()
{
    return "icons/kdiskmark.svg";
}

QString Global::getIconPNGPath()
{
    return "icons/kdiskmark.png";
}

QString Global::getToolTipTemplate()
{
    return QObject::tr("<h1>%1 MiB/s<br/>%2 GiB/s<br/>%3 IOPS<br/>%4 μs</h1>");
}
