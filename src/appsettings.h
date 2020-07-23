#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QObject>
#include <QString>

class AppSettings : public QObject
{
    Q_OBJECT

public:
    enum BenchmarkTest {
        SEQ_1,
        SEQ_2,
        RND_1,
        RND_2
    };

    struct BenchmarkParams {
        int BlockSize; // KiB
        int Queues;
        int Threads;
    };

    enum ComparisonField {
        MBPerSec,
        GBPerSec,
        IOPS,
        Latency,
    } comprasionField = MBPerSec;

    Q_ENUM(ComparisonField)

    AppSettings() {}
    BenchmarkParams getBenchmarkParams(BenchmarkTest test);
    void setBenchmarkParams(BenchmarkTest test, int blockSize, int queues, int threads);
    void restoreDefaultBenchmarkParams();
    void setLoopsCount(int loops);
    int getLoopsCount();
    void setFileSize(int size);
    int getFileSize();
    void setIntervalTime(int intervalTime);
    int getIntervalTime();
    void setDir(QString dir);
    QString getBenchmarkFile();

private:
    int m_loopsCount = 5;
    int m_fileSize = 1024;
    int m_intervalTime = 5;
    QString m_dir;

    const BenchmarkParams m_default_SEQ_1 { 1024,  8,  1 };
    const BenchmarkParams m_default_SEQ_2 { 1024,  1,  1 };
    const BenchmarkParams m_default_RND_1 {    4, 32, 16 };
    const BenchmarkParams m_default_RND_2 {    4,  1,  1 };

    BenchmarkParams m_SEQ_1 = m_default_SEQ_1;
    BenchmarkParams m_SEQ_2 = m_default_SEQ_2;
    BenchmarkParams m_RND_1 = m_default_RND_1;
    BenchmarkParams m_RND_2 = m_default_RND_2;

};

#endif // APPSETTINGS_H
