#include "benchmark.h"

#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>

Benchmark::PerformanceResult Benchmark::startFIO(int loops, int size, int block_size,
                                                 int queue_depth, int threads, const QString rw)
{
    process_ = new QProcess();
    process_->start("fio", QStringList()
                    << "--output-format=json"
                    << "--ioengine=libaio"
                    << "--direct=1"
                    << "--filename=/home/jonmagon/fiotest.tmp"
                    << QString("--name=%1").arg(rw)
                    << QString("--loops=%1").arg(loops)
                    << QString("--size=%1k").arg(size)
                    << QString("--bs=%1k").arg(block_size)
                    << QString("--rw=%1").arg(rw)
                    << QString("--iodepth=%1").arg(queue_depth)
                    << QString("--numjobs=%1").arg(threads)
                    );
    process_->waitForFinished();

    return parseResult();
}

Benchmark::PerformanceResult Benchmark::parseResult()
{
    QJsonDocument jsonResponse = QJsonDocument::fromJson(QString(process_->readAllStandardOutput()).toUtf8());
    QJsonObject jsonObject = jsonResponse.object();
    QJsonArray jobs = jsonObject["jobs"].toArray();

    if (jobs.count() == 0)
        throw 1;

    PerformanceResult result = PerformanceResult();

    QJsonValue job = jobs.takeAt(0);

    if (job["jobname"].toString().contains("read")) {
        QJsonObject readResults = job["read"].toObject();

        result.Bandwidth = readResults.value("bw").toInt() / 1000.0; // to kbytes
        result.IOPS = readResults.value("iops").toDouble();
        result.Latency = readResults["lat_ns"].toObject().value("mean").toDouble() / 1000.0; // from nsec to usec
    } else if (job["jobname"].toString().contains("write")) {
        QJsonObject writeResults = job["write"].toObject();

        result.Bandwidth = writeResults.value("bw").toInt() / 1000.0; // to kbytes
        result.IOPS = writeResults.value("iops").toDouble();
        result.Latency = writeResults["lat_ns"].toObject().value("mean").toDouble() / 1000.0; // from nsec to usec
    }

    return result;
}

void Benchmark::waitTask(int sec)
{
    for (int i = 0; i < sec; i++) {
        emit benchmarkStatusUpdated(tr("Interval Time %1/%2 sec").arg(i).arg(sec));
        QThread::sleep(1);
    }
}

void Benchmark::runBenchmark(QProgressBar *progressBar, Benchmark::Type type, int loops)
{
    switch (type)
    {
    case SEQ1M_Q8T1_Read:
        emit benchmarkStatusUpdated(tr("Sequential Read"));
        emit resultReady(progressBar, startFIO(loops, 16 * 1024, 1024, 8, 1, kRW_READ));
        break;
    case SEQ1M_Q8T1_Write:
        emit benchmarkStatusUpdated(tr("Sequential Write"));
        emit resultReady(progressBar, startFIO(loops, 16 * 1024, 1024, 8, 1, kRW_WRITE));
        break;
    case SEQ1M_Q1T1_Read:
        emit benchmarkStatusUpdated(tr("Sequential Read"));
        emit resultReady(progressBar, startFIO(loops, 16 * 1024, 1024, 1, 1, kRW_READ));
        break;
    case SEQ1M_Q1T1_Write:
        emit benchmarkStatusUpdated(tr("Sequential Write"));
        emit resultReady(progressBar, startFIO(loops, 16 * 1024, 1024, 1, 1, kRW_WRITE));
        break;
    case RND4K_Q32T16_Read:
        emit benchmarkStatusUpdated(tr("Random Read"));
        emit resultReady(progressBar, startFIO(loops, 16 * 1024, 4, 32, 16, kRW_RANDREAD));
        break;
    case RND4K_Q32T16_Write:
        emit benchmarkStatusUpdated(tr("Random Write"));
        emit resultReady(progressBar, startFIO( loops, 16 * 1024, 4, 32, 16, kRW_RANDWRITE));
        break;
    case RND4K_Q1T1_Read:
        emit benchmarkStatusUpdated(tr("Random Read"));
        emit resultReady(progressBar, startFIO(loops, 16 * 1024, 4, 1, 1, kRW_RANDREAD));
        break;
    case RND4K_Q1T1_Write:
        emit benchmarkStatusUpdated(tr("Random Write"));
        emit resultReady(progressBar, startFIO(loops, 16 * 1024, 4, 1, 1, kRW_RANDWRITE));
        break;
    }
}
