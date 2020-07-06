#include "benchmark.h"

#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>
#include <QFile>

#include "appsettings.h"
#include "global.h"

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

QString Benchmark::getFIOVersion()
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
                    << QString("--size=%1m").arg(m_settings->getFileSize())
                    << QString("--bs=%1k").arg(block_size)
                    << QString("--rw=%1").arg(rw)
                    << QString("--iodepth=%1").arg(queue_depth)
                    << QString("--numjobs=%1").arg(threads)
                    );
    m_process->waitForFinished();

    if (m_process->exitStatus() == QProcess::NormalExit) {
        return parseResult();
    }
    else {
        setRunning(false);

        m_process->close();

        delete m_process;

        return PerformanceResult();
    }
}

Benchmark::PerformanceResult Benchmark::parseResult()
{
    QJsonDocument jsonResponse = QJsonDocument::fromJson(QString(m_process->readAllStandardOutput()).toUtf8());
    QJsonObject jsonObject = jsonResponse.object();
    QJsonArray jobs = jsonObject["jobs"].toArray();

    PerformanceResult result = PerformanceResult();

    QString errorOutput = m_process->readAllStandardError();

    if (jobs.count() == 0 && !errorOutput.isEmpty()) {
        setRunning(false);
        emit failed(errorOutput);
    }
    else if (jobs.count() == 0) {
        setRunning(false);
        emit failed("Bad FIO output.");
    }
    else {
        QJsonObject job = jobs.takeAt(0).toObject();

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
            emit failed(errorOutput.mid(errorOutput.simplified().lastIndexOf("=") + 1));
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

    if (!m_running)
        m_process->kill();

    emit runningStateChanged(state);
}

void Benchmark::runBenchmark(QList<QPair<Benchmark::Type, QProgressBar*>> tests)
{
    QListIterator<QPair<Benchmark::Type, QProgressBar*>> iter(tests);
    // Set to 0 all the progressbars for current tests
    while (iter.hasNext()) {
        emit resultReady(iter.next().second, PerformanceResult());
    }

    iter.toFront();

    QPair<Benchmark::Type, QProgressBar*> item;

    while (iter.hasNext() && m_running) {
        item = iter.next();
        switch (item.first)
        {
        case SEQ_1_Read:
            emit benchmarkStatusUpdate(tr("Sequential Read"));
            emit resultReady(item.second, startFIO(m_settings->SEQ_1.BlockSize,
                                                   m_settings->SEQ_1.Queues,
                                                   m_settings->SEQ_1.Threads,
                                                   Global::getRWRead()));
            break;
        case SEQ_1_Write:
            emit benchmarkStatusUpdate(tr("Sequential Write"));
            emit resultReady(item.second, startFIO(m_settings->SEQ_1.BlockSize,
                                                   m_settings->SEQ_1.Queues,
                                                   m_settings->SEQ_1.Threads,
                                                   Global::getRWWrite()));
            break;
        case SEQ_2_Read:
            emit benchmarkStatusUpdate(tr("Sequential Read"));
            emit resultReady(item.second, startFIO(m_settings->SEQ_2.BlockSize,
                                                    m_settings->SEQ_2.Queues,
                                                    m_settings->SEQ_2.Threads,
                                                    Global::getRWRead()));
            break;
        case SEQ_2_Write:
            emit benchmarkStatusUpdate(tr("Sequential Write"));
            emit resultReady(item.second, startFIO(m_settings->SEQ_2.BlockSize,
                                                   m_settings->SEQ_2.Queues,
                                                   m_settings->SEQ_2.Threads,
                                                   Global::getRWWrite()));
            break;
        case RND_1_Read:
            emit benchmarkStatusUpdate(tr("Random Read"));
            emit resultReady(item.second, startFIO(m_settings->RND_1.BlockSize,
                                                   m_settings->RND_1.Queues,
                                                   m_settings->RND_1.Threads,
                                                   Global::getRWRandomRead()));
            break;
        case RND_1_Write:
            emit benchmarkStatusUpdate(tr("Random Write"));
            emit resultReady(item.second, startFIO(m_settings->RND_1.BlockSize,
                                                   m_settings->RND_1.Queues,
                                                   m_settings->RND_1.Threads,
                                                   Global::getRWRandomWrite()));
            break;
        case RND_2_Read:
            emit benchmarkStatusUpdate(tr("Random Read"));
            emit resultReady(item.second, startFIO(m_settings->RND_2.BlockSize,
                                                   m_settings->RND_2.Queues,
                                                   m_settings->RND_2.Threads,
                                                   Global::getRWRandomRead()));
            break;
        case RND_2_Write:
            emit benchmarkStatusUpdate(tr("Random Write"));
            emit resultReady(item.second, startFIO(m_settings->RND_2.BlockSize,
                                                   m_settings->RND_2.Queues,
                                                   m_settings->RND_2.Threads,
                                                   Global::getRWRandomWrite()));
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
