#include "global.h"

#include <QString>
#include <QObject>
#include <QStandardPaths>

QString Global::getIconSVGPath()
{
    return QStandardPaths::locate(QStandardPaths::AppDataLocation,
                                  QStringLiteral("icons/kdiskmark.svg"),
                                  QStandardPaths::LocateFile);
}

QString Global::getIconPNGPath()
{
    return QStandardPaths::locate(QStandardPaths::AppDataLocation,
                                  QStringLiteral("icons/kdiskmark.png"),
                                  QStandardPaths::LocateFile);
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
