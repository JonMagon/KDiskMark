#include "global.h"

#include <QString>
#include <QObject>

int Global::getOutputColumnsCount()
{
    return 78;
}

QString Global::getBenchmarkButtonText(BenchmarkParams params, QString paramsLine)
{
    QString text = QStringLiteral("%1%2%3\n%4")
            .arg(params.Pattern == Global::BenchmarkIOPattern::SEQ ? QStringLiteral("SEQ") : QStringLiteral("RND"))
            .arg(params.BlockSize >= 1024 ? params.BlockSize / 1024 : params.BlockSize)
            .arg(params.BlockSize >= 1024 ? QStringLiteral("M") : QStringLiteral("K"));
    if (paramsLine.isEmpty())
        return text.arg("Q%1T%2").arg(params.Queues).arg(params.Threads);
    else
        return text.arg("(%1)").arg(paramsLine);
}

QString Global::getBenchmarkButtonToolTip(BenchmarkParams params, bool extraLine)
{
    return QObject::tr("<h2>%1 %2 %3<br/>Queues=%4<br/>Threads=%5%6</h2>")
            .arg(params.Pattern == Global::BenchmarkIOPattern::SEQ ? QObject::tr("Sequential") : QObject::tr("Random"))
            .arg(params.BlockSize >= 1024 ? params.BlockSize / 1024 : params.BlockSize)
            .arg(params.BlockSize >= 1024 ? QObject::tr("MiB") : QObject::tr("KiB"))
            .arg(params.Queues).arg(params.Threads).arg(extraLine ? QStringLiteral("<br/>(%1)") : QStringLiteral());
}

QString Global::getToolTipTemplate()
{
    return QObject::tr("<h1>%1 MB/s<br/>%2 GB/s<br/>%3 IOPS<br/>%4 μs</h1>");
}

QString Global::getComparisonLabelTemplate()
{
    return QStringLiteral("<p align=\"center\">%1 [%2]</p>");
}

QString Global::getRWSequentialRead()
{
    return QStringLiteral("read");
}

QString Global::getRWSequentialWrite()
{
    return QStringLiteral("write");
}

QString Global::getRWSequentialMix()
{
    return QStringLiteral("rw");
}

QString Global::getRWRandomRead()
{
    return QStringLiteral("randread");
}

QString Global::getRWRandomWrite()
{
    return QStringLiteral("randwrite");
}

QString Global::getRWRandomMix()
{
    return QStringLiteral("randrw");
}
