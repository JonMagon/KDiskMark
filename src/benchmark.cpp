#include "benchmark.h"

#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>
#include <QFile>

#include <signal.h>

#include "appsettings.h"
#include "global.h"

Benchmark::Benchmark(AppSettings *settings)
{
    m_settings = settings;

    QProcess process;
    process.start("fio", QStringList() << "--version");
    process.waitForFinished();

    m_FIOVersion = process.readAllStandardOutput().simplified();

    process.close();
}

QString Benchmark::getFIOVersion()
{
    return m_FIOVersion;
}

bool Benchmark::isFIODetected()
{
    return m_FIOVersion.indexOf("fio-") == 0;
}

void Benchmark::startFIO(int block_size, int queue_depth, int threads, const QString &rw, const QString &statusMessage)
{
    for (int i = 0; i < m_settings->getLoopsCount(); i++) {
        auto process = std::make_shared<QProcess>();
        process->start("fio", QStringList()
                         << "--output-format=json"
                         << "--ioengine=libaio"
                         << "--direct=1"
                         << "--randrepeat=0"
                         << "--refill_buffers"
                         << "--end_fsync=1"
                         << QStringLiteral("--rwmixread=%1").arg(m_settings->getRandomReadPercentage())
                         << QStringLiteral("--filename=%1").arg(m_settings->getBenchmarkFile())
                         << QStringLiteral("--name=%1").arg(rw)
                         << QStringLiteral("--size=%1m").arg(m_settings->getFileSize())
                         << QStringLiteral("--bs=%1k").arg(block_size)
                         << QStringLiteral("--rw=%1").arg(rw)
                         << QStringLiteral("--iodepth=%1").arg(queue_depth)
                         << QStringLiteral("--numjobs=%1").arg(threads));

        kill(process->processId(), SIGSTOP); // Suspend

        m_processes.push_back(process);
    }

    PerformanceResult totalRead { 0, 0, 0 }, totalWrite { 0, 0, 0 };

    unsigned int loops_count = 0;

    for (auto &process : m_processes) {
        if (!m_running) break;

        emit benchmarkStatusUpdate(statusMessage.arg(loops_count).arg(m_settings->getLoopsCount()));

        kill(process->processId(), SIGCONT); // Resume

        process->waitForFinished(-1);

        if (process->exitStatus() != QProcess::NormalExit) {
            setRunning(false);
        }

        PerformanceResult prepare { 0, 0, 0 };

        if (m_running) {
            loops_count++;

            auto result = parseResult(process);

            PerformanceResult resultRead = result[0];
            totalRead.Bandwidth += resultRead.Bandwidth;
            totalRead.IOPS += resultRead.IOPS;
            totalRead.Latency += resultRead.Latency;

            PerformanceResult resultWrite = result[1];
            totalWrite.Bandwidth += resultWrite.Bandwidth;
            totalWrite.IOPS += resultWrite.IOPS;
            totalWrite.Latency += resultWrite.Latency;

            if (rw.contains("read")) {
                prepare = totalRead;
            } else if (rw.contains("write")) {
                prepare = totalWrite;
            } else if (rw.contains("rw")) {
                float p = m_settings->getRandomReadPercentage();
                prepare = PerformanceResult {
                        (totalRead.Bandwidth * p + totalWrite.Bandwidth * (100.f - p)) / 100.f,
                        (totalRead.IOPS * p + totalWrite.IOPS * (100.f - p)) / 100.f,
                        (totalRead.Latency * p + totalWrite.Latency * (100.f - p)) / 100.f };
            }
        }

        process->close();

        emit resultReady(m_progressBar, loops_count != 0
                ? PerformanceResult {
                  prepare.Bandwidth / loops_count,
                  prepare.IOPS / loops_count,
                  prepare.Latency / loops_count }
                : prepare);
    }

    m_processes.clear();
}

std::array<Benchmark::PerformanceResult, 2> Benchmark::parseResult(const std::shared_ptr<QProcess> process)
{
    QString output = QString(process->readAllStandardOutput());
    QJsonDocument jsonResponse = QJsonDocument::fromJson(output.toUtf8());
    QJsonObject jsonObject = jsonResponse.object();
    QJsonArray jobs = jsonObject["jobs"].toArray();

    PerformanceResult resultRead = PerformanceResult(), resultWrite = PerformanceResult();

    QString errorOutput = process->readAllStandardError();

    int jobsCount = jobs.count();

    if (jobsCount == 0 && !errorOutput.isEmpty()) {
        setRunning(false);
        emit failed(errorOutput);
    }
    else if (jobsCount == 0) {
        setRunning(false);
        emit failed("Bad FIO output.");
    }
    else {

        for (int i = 0; i < jobsCount; i++) {
            QJsonObject job = jobs.takeAt(i).toObject();

            if (job["error"].toInt() == 0) {
                QJsonObject jobRead = job["read"].toObject();
                resultRead.Bandwidth += jobRead.value("bw").toInt() / 1000.0; // to mb
                resultRead.IOPS += jobRead.value("iops").toDouble();
                resultRead.Latency += jobRead["lat_ns"].toObject().value("mean").toDouble() / 1000.0 / jobsCount; // to usec

                QJsonObject jobWrite = job["write"].toObject();
                resultWrite.Bandwidth += jobWrite.value("bw").toInt() / 1000.0; // to mb
                resultWrite.IOPS += jobWrite.value("iops").toDouble();
                resultWrite.Latency += jobWrite["lat_ns"].toObject().value("mean").toDouble() / 1000.0 / jobsCount; // to usec
            }
            else {
                setRunning(false);
                emit failed(errorOutput.mid(errorOutput.simplified().lastIndexOf("=") + 1));
            }
        }
    }

    return std::array<PerformanceResult, 2> { resultRead, resultWrite };
}

void Benchmark::setRunning(bool state)
{
    if (m_running == state)
        return;

    m_running = state;

    if (!m_running) {
        for (auto &process : m_processes) {
            if (process->state() == QProcess::Running || process->state() == QProcess::Starting) {
                kill(process->processId(), SIGINT);
            }
        }
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

        m_progressBar = item.second;

        switch (item.first)
        {
        case SEQ_1_Read:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_1);
            startFIO(params.BlockSize, params.Queues, params.Threads,
                     Global::getRWSequentialRead(), tr("Sequential Read %1/%2"));
            break;
        case SEQ_1_Write:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_1);
            startFIO(params.BlockSize, params.Queues, params.Threads,
                     Global::getRWSequentialWrite(), tr("Sequential Write %1/%2"));
            break;
        case SEQ_1_Mix:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_1);
            startFIO(params.BlockSize, params.Queues, params.Threads,
                     Global::getRWSequentialMix(), tr("Sequential Mix %1/%2"));
            break;
        case SEQ_2_Read:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_2);
            startFIO(params.BlockSize, params.Queues,params.Threads,
                     Global::getRWSequentialRead(), tr("Sequential Read %1/%2"));
            break;
        case SEQ_2_Write:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_2);
            startFIO(params.BlockSize, params.Queues, params.Threads,
                     Global::getRWSequentialWrite(), tr("Sequential Write %1/%2"));
            break;
        case SEQ_2_Mix:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_2);
            startFIO(params.BlockSize, params.Queues, params.Threads,
                     Global::getRWSequentialMix(), tr("Sequential Mix %1/%2"));
            break;
        case RND_1_Read:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_1);
            startFIO(params.BlockSize, params.Queues, params.Threads,
                     Global::getRWRandomRead(), tr("Random Read %1/%2"));
            break;
        case RND_1_Write:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_1);
            startFIO(params.BlockSize, params.Queues, params.Threads,
                     Global::getRWRandomWrite(), tr("Random Write %1/%2"));
            break;
        case RND_1_Mix:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_1);
            startFIO(params.BlockSize, params.Queues, params.Threads,
                     Global::getRWSequentialMix(), tr("Random Mix %1/%2"));
            break;
        case RND_2_Read:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_2);
            startFIO(params.BlockSize, params.Queues, params.Threads,
                     Global::getRWRandomRead(), tr("Random Read %1/%2"));
            break;
        case RND_2_Write:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_2);
            startFIO(params.BlockSize, params.Queues, params.Threads,
                     Global::getRWRandomWrite(), tr("Random Write %1/%2"));
            break;
        case RND_2_Mix:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_2);
            startFIO(params.BlockSize, params.Queues, params.Threads,
                     Global::getRWSequentialMix(), tr("Random Mix %1/%2"));
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
