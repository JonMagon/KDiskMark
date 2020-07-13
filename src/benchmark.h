#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <QProcess>
#include <QStringList>
#include <QString>
#include <QProgressBar>
#include <QObject>

class AppSettings;

class Benchmark : public QObject
{
    Q_OBJECT

    AppSettings *m_settings;
    QProcess *m_process;
    bool m_running;
    QString m_FIOVersion;

public:
    Benchmark(AppSettings *settings);
    QString getFIOVersion();
    bool isFIODetected();

    enum Type {
        SEQ_1_Read,
        SEQ_1_Write,
        SEQ_2_Read,
        SEQ_2_Write,
        RND_1_Read,
        RND_1_Write,
        RND_2_Read,
        RND_2_Write
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
    void runBenchmark(QList<QPair<Benchmark::Type, QProgressBar*>> tests);
    void setRunning(bool state);

signals:
    void benchmarkStatusUpdate(const QString &name);
    void resultReady(QProgressBar *progressBar, const Benchmark::PerformanceResult &result);
    void failed(const QString &error);
    void finished();
    void runningStateChanged(bool state);
};

Q_DECLARE_METATYPE(Benchmark::Type)
Q_DECLARE_METATYPE(Benchmark::PerformanceResult)

#endif // BENCHMARK_H
