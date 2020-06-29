#include "benchmark.h"

#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>

Benchmark::Benchmark()
{
    m_process = new QProcess();
    m_process->start("fio", QStringList() << "--version");
    m_process->waitForFinished();
    FIOVersion = m_process->readAllStandardOutput().simplified();
    m_process->close();

    delete m_process;
}

Benchmark::PerformanceResult Benchmark::startFIO(int loops, int size, int block_size,
                                                 int queue_depth, int threads, const QString rw)
{
    m_process = new QProcess();
    m_process->start("fio", QStringList()
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
    m_process->waitForFinished();

    return parseResult();
}

Benchmark::PerformanceResult Benchmark::parseResult()
{
    QJsonDocument jsonResponse = QJsonDocument::fromJson(QString(m_process->readAllStandardOutput()).toUtf8());
    QJsonObject jsonObject = jsonResponse.object();
    QJsonArray jobs = jsonObject["jobs"].toArray();

    if (jobs.count() == 0)
        throw 1;

    PerformanceResult result = PerformanceResult();

    QJsonValue job = jobs.takeAt(0);

    if (job["jobname"].toString().contains("read")) {
        QJsonObject readResults = job["read"].toObject();

        result.Bandwidth = readResults.value("bw").toInt() / 1024.0; // to kib
        result.IOPS = readResults.value("iops").toDouble();
        result.Latency = readResults["lat_ns"].toObject().value("mean").toDouble() / 1000.0; // from nsec to usec
    }
    else if (job["jobname"].toString().contains("write")) {
        QJsonObject writeResults = job["write"].toObject();

        result.Bandwidth = writeResults.value("bw").toInt() / 1024.0; // to kib
        result.IOPS = writeResults.value("iops").toDouble();
        result.Latency = writeResults["lat_ns"].toObject().value("mean").toDouble() / 1000.0; // from nsec to usec
    }

    m_process->close();

    delete m_process;

    return result;
}

void Benchmark::runBenchmark(QMap<Benchmark::Type, QProgressBar*> tests, int loops, int intervalTime)
{
    bool state = true;
    QMapIterator<Benchmark::Type, QProgressBar*> iter(tests);

    // set to 0 all the progressbars for current tests
    while (iter.hasNext()) {
        iter.next();
        emit resultReady(iter.value(), PerformanceResult());
    }

    iter.toFront();

    while (iter.hasNext()) {

        emit isRunning(&state);
        if (!state) {
            emit finished();
            return;
        }

        iter.next();
        switch (iter.key())
        {
        case SEQ1M_Q8T1_Read:
            emit benchmarkStatusUpdated(tr("Sequential Read"));
            emit resultReady(iter.value(), startFIO(loops, 16 * 1024, 1024, 8, 1, kRW_READ));
            break;
        case SEQ1M_Q8T1_Write:
            emit benchmarkStatusUpdated(tr("Sequential Write"));
            emit resultReady(iter.value(), startFIO(loops, 16 * 1024, 1024, 8, 1, kRW_WRITE));
            break;
        case SEQ1M_Q1T1_Read:
            emit benchmarkStatusUpdated(tr("Sequential Read"));
            emit resultReady(iter.value(), startFIO(loops, 16 * 1024, 1024, 1, 1, kRW_READ));
            break;
        case SEQ1M_Q1T1_Write:
            emit benchmarkStatusUpdated(tr("Sequential Write"));
            emit resultReady(iter.value(), startFIO(loops, 16 * 1024, 1024, 1, 1, kRW_WRITE));
            break;
        case RND4K_Q32T16_Read:
            emit benchmarkStatusUpdated(tr("Random Read"));
            emit resultReady(iter.value(), startFIO(loops, 16 * 1024, 4, 32, 16, kRW_RANDREAD));
            break;
        case RND4K_Q32T16_Write:
            emit benchmarkStatusUpdated(tr("Random Write"));
            emit resultReady(iter.value(), startFIO(loops, 16 * 1024, 4, 32, 16, kRW_RANDWRITE));
            break;
        case RND4K_Q1T1_Read:
            emit benchmarkStatusUpdated(tr("Random Read"));
            emit resultReady(iter.value(), startFIO(loops, 16 * 1024, 4, 1, 1, kRW_RANDREAD));
            break;
        case RND4K_Q1T1_Write:
            emit benchmarkStatusUpdated(tr("Random Write"));
            emit resultReady(iter.value(), startFIO(loops, 16 * 1024, 4, 1, 1, kRW_RANDWRITE));
            break;
        }

        if (iter.hasNext()) {
            for (int i = 0; i < intervalTime; i++, emit isRunning(&state)) {
                if (!state) {
                    emit finished();
                    return;
                }

                emit benchmarkStatusUpdated(tr("Interval Time %1/%2 sec").arg(i).arg(intervalTime));
                QThread::sleep(1);
            }
        }
    }

    emit finished();
}
