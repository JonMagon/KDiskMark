#ifndef FIO_H
#define FIO_H

#include <QProcess>
#include <QStringList>
#include <QString>
#include <QProgressBar>
#include <QObject>

class Benchmark : public QObject
{
    Q_OBJECT

public:
    enum Type {
        SEQ1M_Q8T1_Read,
        SEQ1M_Q8T1_Write,
        SEQ1M_Q1T1_Read,
        SEQ1M_Q1T1_Write,
        RND4K_Q32T16_Read,
        RND4K_Q32T16_Write,
        RND4K_Q1T1_Read,
        RND4K_Q1T1_Write
    };

    struct PerformanceResult
    {
        float Bandwidth;
        float IOPS;
        float Latency;
    };

private:
    const QString kRW_READ = "read";
    const QString kRW_WRITE = "write";
    const QString kRW_RANDREAD = "randread";
    const QString kRW_RANDWRITE = "randwrite";

    QProcess *process_;
    QStringList *args_;
    PerformanceResult startFIO(int loops, int size, int block_size, int queue_depth,
                               int threads, const QString rw);
    PerformanceResult parseResult();

public slots:
    void runBenchmark(QProgressBar *progressBar, Benchmark::Type type, int loops);
    void waitTask(int sec);

signals:
    void benchmarkStatusUpdated(const QString &name);
    void resultReady(QProgressBar *progressBar, const Benchmark::PerformanceResult &result);
};

Q_DECLARE_METATYPE(Benchmark::Type);
Q_DECLARE_METATYPE(Benchmark::PerformanceResult);

#endif // FIO_H
