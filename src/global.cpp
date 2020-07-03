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
    return QObject::tr("<h1>%1 MB/s<br/>%2 GB/s<br/>%3 IOPS<br/>%4 μs</h1>");
}

QString Global::getComparisonLabelTemplate()
{
    return "<p align=\"center\">%1 [%2]</p>";
}

int Global::getOutputColumnsCount()
{
    return 78;
}
