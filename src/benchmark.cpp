#include "benchmark.h"

#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>
#include <QFile>

#include <signal.h>

#include <KAuth>
#include <KAuthAction>

#include "appsettings.h"
#include "global.h"

Benchmark::Benchmark(AppSettings *settings)
{
    m_running = false;
    m_settings = settings;

    QProcess process;
    process.start("fio", {"--version"});
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
    emit benchmarkStatusUpdate(tr("Preparing..."));

    KAuth::Action dropCacheAction("org.jonmagon.kdiskmark.dropcache");
    dropCacheAction.setHelperId("org.jonmagon.kdiskmark");
    QVariantMap args; args["check"] = true; dropCacheAction.setArguments(args);
    KAuth::ExecuteJob* dropCacheJob = dropCacheAction.execute();

    if (m_settings->shouldFlushCache()) {
        dropCacheJob->exec();

        if (dropCacheAction.status() != KAuth::Action::AuthorizedStatus) {
            if (dropCacheJob->error() && !dropCacheJob->errorText().isEmpty()) {
                emit failed(dropCacheJob->errorText());
            }
            setRunning(false);
            return;
        }
    }
    else {
        dropCacheJob->~ExecuteJob();
    }

    args["check"] = false; dropCacheAction.setArguments(args);

    auto prepareProcess = std::make_shared<QProcess>();
    prepareProcess->start("fio", QStringList()
                          << "--create_only=1"
                          << QStringLiteral("--filename=%1").arg(m_settings->getBenchmarkFile())
                          << QStringLiteral("--size=%1m").arg(m_settings->getFileSize())
                          << QStringLiteral("--name=%1").arg(rw));

    prepareProcess->waitForFinished(-1);
    prepareProcess->close();

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

    unsigned int index = 0;

    for (auto &process : m_processes) {
        if (!m_running) break;

        emit benchmarkStatusUpdate(statusMessage.arg(index).arg(m_settings->getLoopsCount()));

        if (m_settings->shouldFlushCache()) {
            dropCacheJob = dropCacheAction.execute();
            dropCacheJob->exec();
        }

        kill(process->processId(), SIGCONT); // Resume

        process->waitForFinished(-1);

        if (process->exitStatus() != QProcess::NormalExit) {
            setRunning(false);
        }

        if (m_running) {
            index++;

            auto result = parseResult(process);

            switch (m_settings->performanceProfile)
            {
                case AppSettings::PerformanceProfile::Default:
                    totalRead  += result.read;
                    totalWrite += result.write;
                break;
                case AppSettings::PerformanceProfile::Peak:
                case AppSettings::PerformanceProfile::RealWorld:
                    totalRead.updateWithBetterValues(result.read);
                    totalWrite.updateWithBetterValues(result.write);
                break;
            }
        }

        process->close();

        if (rw.contains("read")) {
            sendResult(totalRead, index);
        }
        else if (rw.contains("write")) {
            sendResult(totalWrite, index);
        }
        else if (rw.contains("rw")) {
            float p = m_settings->getRandomReadPercentage();
            sendResult((totalRead * p + totalWrite * (100.f - p)) / 100.f, index);
        }
    }

    m_processes.clear();
}

void Benchmark::sendResult(const Benchmark::PerformanceResult &result, const int index)
{
    if (m_settings->performanceProfile == AppSettings::PerformanceProfile::Default) {
        for (auto progressBar : m_progressBars) {
            emit resultReady(progressBar, result / index);
        }
    }
    else {
        for (auto progressBar : m_progressBars) {
            emit resultReady(progressBar, result);
        }
    }
}

Benchmark::ParsedJob Benchmark::parseResult(const std::shared_ptr<QProcess> process)
{
    QString output = QString(process->readAllStandardOutput());
    QJsonDocument jsonResponse = QJsonDocument::fromJson(output.toUtf8());
    QJsonObject jsonObject = jsonResponse.object();
    QJsonArray jobs = jsonObject["jobs"].toArray();

    ParsedJob parsedJob {{0, 0, 0}, {0, 0, 0}};

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
                parsedJob.read.Bandwidth += jobRead.value("bw").toInt() / 1000.0; // to mb
                parsedJob.read.IOPS += jobRead.value("iops").toDouble();
                parsedJob.read.Latency += jobRead["lat_ns"].toObject().value("mean").toDouble() / 1000.0 / jobsCount; // to usec

                QJsonObject jobWrite = job["write"].toObject();
                parsedJob.write.Bandwidth += jobWrite.value("bw").toInt() / 1000.0; // to mb
                parsedJob.write.IOPS += jobWrite.value("iops").toDouble();
                parsedJob.write.Latency += jobWrite["lat_ns"].toObject().value("mean").toDouble() / 1000.0 / jobsCount; // to usec
            }
            else {
                setRunning(false);
                emit failed(errorOutput.mid(errorOutput.simplified().lastIndexOf("=") + 1));
            }
        }
    }

    return parsedJob;
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

void Benchmark::runBenchmark(QList<QPair<Benchmark::Type, QVector<QProgressBar*>>> tests)
{
    QListIterator<QPair<Benchmark::Type, QVector<QProgressBar*>>> iter(tests);
    // Set to 0 all the progressbars for current tests
    while (iter.hasNext()) {
        auto progressBars = iter.next().second;
        for (auto obj : progressBars) {
            emit resultReady(obj, PerformanceResult());
        }
    }

    iter.toFront();

    AppSettings::BenchmarkParams params;

    QPair<Benchmark::Type, QVector<QProgressBar*>> item;

    while (iter.hasNext() && m_running) {
        item = iter.next();

        m_progressBars = item.second;

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
