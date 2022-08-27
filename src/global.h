#ifndef GLOBAL_H
#define GLOBAL_H

#include <QObject>
#include <QString>

namespace Global
{
    Q_NAMESPACE

    enum BenchmarkTest {
        Test_1,
        Test_2,
        Test_3,
        Test_4
    };
    Q_ENUM_NS(BenchmarkTest)

    enum BenchmarkIOReadWrite {
        Read,
        Write,
        Mix
    };

    enum BenchmarkIOPattern {
        SEQ,
        RND
    };
    Q_ENUM_NS(BenchmarkIOPattern)

    struct BenchmarkParams {
        BenchmarkIOPattern Pattern;
        int BlockSize; // KiB
        int Queues;
        int Threads;

        bool operator==(const BenchmarkParams& rhs)
        {
            return this->Pattern == rhs.Pattern && this->BlockSize == rhs.BlockSize &&
                   this->Queues  == rhs.Queues  && this->Threads   == rhs.Threads;
        }
    };

    enum PerformanceProfile {
        Default,
        Peak,
        RealWorld,
        Demo
    };
    Q_ENUM_NS(PerformanceProfile)

    enum BenchmarkPreset {
        Standard,
        NVMe_SSD
    };

    enum BenchmarkMode {
        ReadWriteMix,
        ReadMix,
        WriteMix
    };

    enum BenchmarkTestData {
        Random,
        Zeros
    };

    enum ComparisonUnit {
        MBPerSec,
        GBPerSec,
        IOPS,
        Latency,
    };
    Q_ENUM_NS(ComparisonUnit)

    enum Theme {
        UseFusion,
        StyleSheetLight,
        StyleSheetDark,
        DoNotApply,
    };

    int getOutputColumnsCount();
    QString getBenchmarkButtonText(BenchmarkParams params, QString paramsLine = QStringLiteral());
    QString getBenchmarkButtonToolTip(BenchmarkParams params, bool extraField = false);
    QString getToolTipTemplate();
    QString getComparisonLabelTemplate();
    QString getRWSequentialRead();
    QString getRWSequentialWrite();
    QString getRWSequentialMix();
    QString getRWRandomRead();
    QString getRWRandomWrite();
    QString getRWRandomMix();
}

#endif // GLOBAL_H
