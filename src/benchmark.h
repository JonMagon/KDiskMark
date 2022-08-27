#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <QProcess>
#include <QStringList>
#include <QString>
#include <QProgressBar>
#include <QObject>

#include <memory>

#include "appsettings.h"

struct HelperPrivate;

class Benchmark : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Benchmark)

public:
    Benchmark();
    ~Benchmark();

    QString getFIOVersion();
    bool isFIODetected();

    void setDir(const QString &dir);
    QString getBenchmarkFile();

    void setRunning(bool state);
    bool isRunning();

    bool listStorages();

    void runBenchmark(QList<QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>>> tests);

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

        void updateWithBetterValues(const PerformanceResult& result) {
            Bandwidth = Bandwidth < result.Bandwidth ? result.Bandwidth : Bandwidth;
            IOPS = IOPS < result.IOPS ? result.IOPS : IOPS;
            if (Latency == 0) Latency = result.Latency;
            Latency = Latency > result.Latency ? result.Latency : Latency;
        }
    };

    struct ParsedJob
    {
        PerformanceResult read, write;
    };

    struct Storage
    {
        QString path;
        qlonglong bytesTotal;
        qlonglong bytesOccupied;
    };

private:
    bool m_running;
    QString m_FIOVersion;
    QVector<QProgressBar*> m_progressBars;
    QString m_dir;

    QProcess *m_process;

private:
    void startTest(int blockSize, int queueDepth, int threads, const QString &rw, const QString &statusMessage);
    Benchmark::ParsedJob parseResult(const QString &output, const QString &errorOutput);
    void sendResult(const Benchmark::PerformanceResult &result, const int index);

    bool prepareFile(const QString &benchmarkFile, int fileSize, const QString &rw);
    bool flushPageCache();

signals:
    void mountPointsListReady(const QVector<Storage> &storages);
    void benchmarkStatusUpdate(const QString &name);
    void resultReady(QProgressBar *progressBar, const Benchmark::PerformanceResult &result);
    void failed(const QString &error);
    void finished();
    void runningStateChanged(bool state);
};

#endif // BENCHMARK_H
