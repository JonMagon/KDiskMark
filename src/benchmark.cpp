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
                     << QStringLiteral("--filename=%1").arg(m_settings->getBenchmarkFile())
                     << QStringLiteral("--name=%1").arg(rw)
                     << QStringLiteral("--loops=%1").arg(m_settings->getLoopsCount())
                     << QStringLiteral("--size=%1m").arg(m_settings->getFileSize())
                     << QStringLiteral("--bs=%1k").arg(block_size)
                     << QStringLiteral("--rw=%1").arg(rw)
                     << QStringLiteral("--iodepth=%1").arg(queue_depth)
                     << QStringLiteral("--numjobs=%1").arg(threads));

    m_process->waitForFinished(-1);

    if (m_process->exitStatus() == QProcess::NormalExit && m_running) {
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
    QString output = QString(m_process->readAllStandardOutput());
    QJsonDocument jsonResponse = QJsonDocument::fromJson(output.toUtf8());
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

                result.Bandwidth = readResults.value("bw").toInt() / 1000.0; // to kb
                result.IOPS = readResults.value("iops").toDouble();
                result.Latency = readResults["lat_ns"].toObject().value("mean").toDouble() / 1000.0; // to usec
            }
            else if (job["jobname"].toString().contains("write")) {
                QJsonObject writeResults = job["write"].toObject();

                result.Bandwidth = writeResults.value("bw").toInt() / 1000.0; // to kb
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

    if (!m_running && m_process->state() == QProcess::Running) {
        QProcess *kill_process = new QProcess;
        kill_process->start("kill", QStringList()
                            << "-SIGINT"
                            << QString::number(m_process->processId()));

        kill_process->waitForFinished();

        kill_process->close();

        delete kill_process;
    }

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

    AppSettings::BenchmarkParams params;

    QPair<Benchmark::Type, QProgressBar*> item;

    while (iter.hasNext() && m_running) {
        item = iter.next();
        switch (item.first)
        {
        case SEQ_1_Read:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_1);
            emit benchmarkStatusUpdate(tr("Sequential Read"));
            emit resultReady(item.second, startFIO(params.BlockSize,
                                                   params.Queues,
                                                   params.Threads,
                                                   Global::getRWSequentialRead()));
            break;
        case SEQ_1_Write:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_1);
            emit benchmarkStatusUpdate(tr("Sequential Write"));
            emit resultReady(item.second, startFIO(params.BlockSize,
                                                   params.Queues,
                                                   params.Threads,
                                                   Global::getRWSequentialWrite()));
            break;
        case SEQ_2_Read:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_2);
            emit benchmarkStatusUpdate(tr("Sequential Read"));
            emit resultReady(item.second, startFIO(params.BlockSize,
                                                   params.Queues,
                                                   params.Threads,
                                                   Global::getRWSequentialRead()));
            break;
        case SEQ_2_Write:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_2);
            emit benchmarkStatusUpdate(tr("Sequential Write"));
            emit resultReady(item.second, startFIO(params.BlockSize,
                                                   params.Queues,
                                                   params.Threads,
                                                   Global::getRWSequentialWrite()));
            break;
        case RND_1_Read:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_1);
            emit benchmarkStatusUpdate(tr("Random Read"));
            emit resultReady(item.second, startFIO(params.BlockSize,
                                                   params.Queues,
                                                   params.Threads,
                                                   Global::getRWRandomRead()));
            break;
        case RND_1_Write:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_1);
            emit benchmarkStatusUpdate(tr("Random Write"));
            emit resultReady(item.second, startFIO(params.BlockSize,
                                                   params.Queues,
                                                   params.Threads,
                                                   Global::getRWRandomWrite()));
            break;
        case RND_2_Read:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_2);
            emit benchmarkStatusUpdate(tr("Random Read"));
            emit resultReady(item.second, startFIO(params.BlockSize,
                                                   params.Queues,
                                                   params.Threads,
                                                   Global::getRWRandomRead()));
            break;
        case RND_2_Write:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_2);
            emit benchmarkStatusUpdate(tr("Random Write"));
            emit resultReady(item.second, startFIO(params.BlockSize,
                                                   params.Queues,
                                                   params.Threads,
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
