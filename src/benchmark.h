#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <QProcess>
#include <QStringList>
#include <QString>
#include <QProgressBar>
#include <QObject>
#include <QMap>

class AppSettings;

class Benchmark : public QObject
{
    Q_OBJECT

    const QString kRW_READ = "read";
    const QString kRW_WRITE = "write";
    const QString kRW_RANDREAD = "randread";
    const QString kRW_RANDWRITE = "randwrite";

    AppSettings *m_settings;
    QProcess *m_process;
    bool m_running;
    QString m_FIOVersion;

public:
    Benchmark(AppSettings *settings);
    QString FIOVersion();
    bool isFIODetected();

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
    PerformanceResult startFIO(int block_size, int queue_depth, int threads, const QString rw);
    PerformanceResult parseResult();

public slots:
    // TODO: pass all params except tests as one object
    void runBenchmark(QMap<Benchmark::Type, QProgressBar*> tests);
    void setRunning(bool state);

signals:
    void benchmarkStatusUpdate(const QString &name);
    void resultReady(QProgressBar *progressBar, const Benchmark::PerformanceResult &result);
    void failed(const QString &error);
    void finished();
    void runningStateChanged(bool state);
};

Q_DECLARE_METATYPE(Benchmark::Type);
Q_DECLARE_METATYPE(Benchmark::PerformanceResult);

#endif // BENCHMARK_H
