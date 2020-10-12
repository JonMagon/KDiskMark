#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <QProcess>
#include <QStringList>
#include <QString>
#include <QProgressBar>
#include <QObject>

#include <memory>

class AppSettings;

class Benchmark : public QObject
{
    Q_OBJECT

    AppSettings *m_settings;
    std::vector<std::shared_ptr<QProcess>> m_processes;
    bool m_running;
    QString m_FIOVersion;
    QVector<QProgressBar*> m_progressBars;

public:
    Benchmark(AppSettings *settings);
    QString getFIOVersion();
    bool isFIODetected();

    enum Type {
        SEQ_1_Read,
        SEQ_1_Write,
        SEQ_1_Mix,
        SEQ_2_Read,
        SEQ_2_Write,
        SEQ_2_Mix,
        RND_1_Read,
        RND_1_Write,
        RND_1_Mix,
        RND_2_Read,
        RND_2_Write,
        RND_2_Mix
    };

    struct PerformanceResult
    {
        float Bandwidth;
        float IOPS;
        float Latency;

        PerformanceResult operator+ (const PerformanceResult& rhs)
        {
            return *this += rhs;
        }

        PerformanceResult operator+= (const PerformanceResult& rhs)
        {
            Bandwidth += rhs.Bandwidth;
            IOPS += rhs.IOPS;
            Latency += rhs.Latency;
            return *this;
        }

        PerformanceResult operator/ (const unsigned int rhs) const
        {
            if (rhs == 0) return *this;

            return PerformanceResult { Bandwidth / rhs, IOPS / rhs, Latency / rhs };
        }

        PerformanceResult operator* (const unsigned int rhs) const
        {
            return PerformanceResult { Bandwidth * rhs, IOPS * rhs, Latency * rhs };
        }

        bool operator< (const PerformanceResult& rhs) {
            return Bandwidth < rhs.Bandwidth;
        }
    };

    struct ParsedJob
    {
        PerformanceResult read, write;
    };

private:
    void startFIO(int block_size, int queue_depth, int threads, const QString &rw, const QString &statusMessage);
    Benchmark::ParsedJob parseResult(const std::shared_ptr<QProcess> process);
    void sendResult(const Benchmark::PerformanceResult &result, const int index);

public slots:
    void runBenchmark(QList<QPair<Benchmark::Type, QVector<QProgressBar*>>> tests);
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
