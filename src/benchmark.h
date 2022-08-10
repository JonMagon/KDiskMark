#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <QProcess>
#include <QStringList>
#include <QString>
#include <QProgressBar>
#include <QObject>
#include <QThread>

#include <KAuth>

#include <memory>

class AppSettings;
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

public:
    ~Benchmark();

private:
    AppSettings *m_settings;
    bool m_running;
    QString m_FIOVersion;
    QVector<QProgressBar*> m_progressBars;

    DevJonmagonKdiskmarkHelperInterface* helperInterface();

private:
    DBusThread *m_thread;

    // KAuth
    KAuth::ExecuteJob *m_job;
    bool m_helperStarted = false;

public:
    Benchmark(AppSettings *settings);
    QString getFIOVersion();
    bool isFIODetected();

    void setRunning(bool state);
    bool isRunning();

    // KAuth
    bool startHelper();
    void stopHelper();

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

public:
    bool listStorages(); // TEST !


    void runBenchmark(QList<QPair<Benchmark::Type, QVector<QProgressBar*>>> tests); // TEST !

private:
    bool flushPageCache(); // TEST !
    bool prepareFile(const QString &benchmarkFile, int fileSize, const QString &rw); // TEST !

    void startTest(int blockSize, int queueDepth, int threads, const QString &rw, const QString &statusMessage);
    Benchmark::ParsedJob parseResult(const QString &output, const QString &errorOutput);
    void sendResult(const Benchmark::PerformanceResult &result, const int index);

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
