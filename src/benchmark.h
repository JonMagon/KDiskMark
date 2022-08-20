#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <QProcess>
#include <QStringList>
#include <QString>
#include <QProgressBar>
#include <QObject>
#include <QThread>

#include <memory>

#include "appsettings.h"

class QDBusPendingCall;
class DevJonmagonKdiskmarkHelperInterface;

struct HelperPrivate;

class DBusThread : public QThread
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "dev.jonmagon.kdiskmark.applicationinterface")

    void run() override;
};

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

    void setMixed(bool state);
    bool isMixed();

    void setRunning(bool state);
    bool isRunning();

    // KAuth
    bool startHelper();
    void stopHelper();

    bool listStorages();

    enum ComparisonField {
        MBPerSec,
        GBPerSec,
        IOPS,
        Latency,
    } comprasionField = MBPerSec;

    Q_ENUM(ComparisonField)

    Global::PerformanceProfile performanceProfile = Global::PerformanceProfile::Default;
    Global::BenchmarkMode benchmarkMode = Global::BenchmarkMode::ReadWriteMix;

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

    QVector<Storage> storages;

private:
    bool m_running;
    QString m_FIOVersion;
    QVector<QProgressBar*> m_progressBars;
    QString m_dir;
    bool m_mixedState = false;

    DevJonmagonKdiskmarkHelperInterface* helperInterface();
    DBusThread *m_thread;

    // KAuth
    bool m_helperStarted = false;

private:
    void startTest(int blockSize, int queueDepth, int threads, const QString &rw, const QString &statusMessage);
    Benchmark::ParsedJob parseResult(const QString &output, const QString &errorOutput);
    void sendResult(const Benchmark::PerformanceResult &result, const int index);

    bool prepareFile(const QString &benchmarkFile, int fileSize, const QString &rw);
    bool flushPageCache();

    void dbusWaitForFinish(QDBusPendingCall pcall);

signals:
    void benchmarkStatusUpdate(const QString &name);
    void resultReady(QProgressBar *progressBar, const Benchmark::PerformanceResult &result);
    void failed(const QString &error);
    void finished();
    void runningStateChanged(bool state);
};

Q_DECLARE_METATYPE(Benchmark::PerformanceResult)

#endif // BENCHMARK_H
