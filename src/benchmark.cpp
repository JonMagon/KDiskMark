#include "benchmark.h"

#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>
#include <QFile>

#include "appsettings.h"

Benchmark::Benchmark(AppSettings *settings)
{
    m_settings = settings;

    m_process = new QProcess();
    m_process->start("fio", QStringList() << "--version");
    m_process->waitForFinished();

    m_FIOVersion = m_process->readAllStandardOutput().simplified();

    m_process->close();

    delete m_process;
}

QString Benchmark::FIOVersion()
{
    return m_FIOVersion;
}

bool Benchmark::isFIODetected()
{
    return m_FIOVersion.indexOf("fio-") == 0;
}

Benchmark::PerformanceResult Benchmark::startFIO(int block_size, int queue_depth, int threads, const QString rw)
{
    m_process = new QProcess();
    m_process->start("fio", QStringList()
                    << "--output-format=json"
                    << "--ioengine=libaio"
                    << "--direct=1"
                    << QString("--filename=%1").arg(m_settings->getBenchmarkFile())
                    << QString("--name=%1").arg(rw)
                    << QString("--loops=%1").arg(m_settings->getLoopsCount())
                    << QString("--size=%1k").arg(16 * 1024)
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

    PerformanceResult result = PerformanceResult();

    if (jobs.count() == 0) {
        setRunning(false);
        emit failed("Bad FIO output.");
    }
    else {
        QJsonValue job = jobs.takeAt(0);

        if (job["error"].toInt() == 0) {
            if (job["jobname"].toString().contains("read")) {
                QJsonObject readResults = job["read"].toObject();

                result.Bandwidth = readResults.value("bw").toInt() / 1024.0; // to kib
                result.IOPS = readResults.value("iops").toDouble();
                result.Latency = readResults["lat_ns"].toObject().value("mean").toDouble() / 1000.0; // to usec
            }
            else if (job["jobname"].toString().contains("write")) {
                QJsonObject writeResults = job["write"].toObject();

                result.Bandwidth = writeResults.value("bw").toInt() / 1024.0; // to kib
                result.IOPS = writeResults.value("iops").toDouble();
                result.Latency = writeResults["lat_ns"].toObject().value("mean").toDouble() / 1000.0; // to usec
            }
        }
        else {
            setRunning(false);
            QString errorOutput = m_process->readAllStandardError().simplified();
            emit failed(errorOutput.mid(errorOutput.lastIndexOf("=") + 1));
        }
    }

    m_process->close();

    delete m_process;

    return result;
}

void Benchmark::setRunning(bool state)
{
    if (m_running == state)
        return;

    m_running = state;

    emit runningStateChanged(state);
}

void Benchmark::runBenchmark(QMap<Benchmark::Type, QProgressBar*> tests)
{
    QMapIterator<Benchmark::Type, QProgressBar*> iter(tests);

    // Set to 0 all the progressbars for current tests
    while (iter.hasNext()) {
        iter.next();
        emit resultReady(iter.value(), PerformanceResult());
    }

    iter.toFront();

    while (iter.hasNext() && m_running) {
        iter.next();
        switch (iter.key())
        {
        case SEQ1M_Q8T1_Read:
            emit benchmarkStatusUpdate(tr("Sequential Read"));
            emit resultReady(iter.value(), startFIO(m_settings->SEQ_1.BlockSize,
                                                    m_settings->SEQ_1.Queues,
                                                    m_settings->SEQ_1.Threads,
                                                    kRW_READ));
            break;
        case SEQ1M_Q8T1_Write:
            emit benchmarkStatusUpdate(tr("Sequential Write"));
            emit resultReady(iter.value(), startFIO(m_settings->SEQ_1.BlockSize,
                                                    m_settings->SEQ_1.Queues,
                                                    m_settings->SEQ_1.Threads,
                                                    kRW_WRITE));
            break;
        case SEQ1M_Q1T1_Read:
            emit benchmarkStatusUpdate(tr("Sequential Read"));
            emit resultReady(iter.value(), startFIO(m_settings->SEQ_2.BlockSize,
                                                    m_settings->SEQ_2.Queues,
                                                    m_settings->SEQ_2.Threads,
                                                    kRW_READ));
            break;
        case SEQ1M_Q1T1_Write:
            emit benchmarkStatusUpdate(tr("Sequential Write"));
            emit resultReady(iter.value(), startFIO(m_settings->SEQ_2.BlockSize,
                                                    m_settings->SEQ_2.Queues,
                                                    m_settings->SEQ_2.Threads,
                                                    kRW_WRITE));
            break;
        case RND4K_Q32T16_Read:
            emit benchmarkStatusUpdate(tr("Random Read"));
            emit resultReady(iter.value(), startFIO(m_settings->RND_1.BlockSize,
                                                    m_settings->RND_1.Queues,
                                                    m_settings->RND_1.Threads,
                                                    kRW_RANDREAD));
            break;
        case RND4K_Q32T16_Write:
            emit benchmarkStatusUpdate(tr("Random Write"));
            emit resultReady(iter.value(), startFIO(m_settings->RND_1.BlockSize,
                                                    m_settings->RND_1.Queues,
                                                    m_settings->RND_1.Threads,
                                                    kRW_RANDWRITE));
            break;
        case RND4K_Q1T1_Read:
            emit benchmarkStatusUpdate(tr("Random Read"));
            emit resultReady(iter.value(), startFIO(m_settings->RND_2.BlockSize,
                                                    m_settings->RND_2.Queues,
                                                    m_settings->RND_2.Threads,
                                                    kRW_RANDREAD));
            break;
        case RND4K_Q1T1_Write:
            emit benchmarkStatusUpdate(tr("Random Write"));
            emit resultReady(iter.value(), startFIO(m_settings->RND_2.BlockSize,
                                                    m_settings->RND_2.Queues,
                                                    m_settings->RND_2.Threads,
                                                    kRW_RANDWRITE));
            break;
        }

        if (iter.hasNext()) {
            for (int i = 0; i < m_settings->getIntervalTime() && m_running; i++) {
                emit benchmarkStatusUpdate(tr("Interval Time %1/%2 sec").arg(i).arg(m_settings->getIntervalTime()));
                QThread::sleep(1);
            }
        }
    }

    QFile(m_settings->getBenchmarkFile()).remove();

    setRunning(false);
    emit finished();
}
