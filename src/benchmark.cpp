#include "benchmark.h"

#include <QFile>
#include <QEventLoop>
#include <QTimer>
#include <QStorageInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "global.h"

Benchmark::Benchmark()
{
    m_running = false;

    QProcess process;
    process.start("fio", {"--version"});
    process.waitForFinished();

    m_FIOVersion = process.readAllStandardOutput().simplified();

    process.close();
}

Benchmark::~Benchmark() {}

QString Benchmark::getFIOVersion()
{
    return m_FIOVersion;
}

bool Benchmark::isFIODetected()
{
    return m_FIOVersion.indexOf("fio-") == 0;
}

void Benchmark::setDir(const QString &dir)
{
    m_dir = dir;
}

QString Benchmark::getBenchmarkFile()
{
    if (m_dir.isNull())
        return QString();

    if (m_dir.endsWith("/")) {
        return m_dir + ".kdiskmark.tmp";
    }
    else {
        return m_dir + "/.kdiskmark.tmp";
    }
}

void Benchmark::startTest(int blockSize, int queueDepth, int threads, const QString &rw, const QString &statusMessage)
{
    const AppSettings settings;

    PerformanceResult totalRead { 0, 0, 0 }, totalWrite { 0, 0, 0 };

    unsigned int index = 0;

    for (int i = 0; i < settings.getLoopsCount(); i++) {
        if (!m_running) break;

        emit benchmarkStatusUpdate(statusMessage.arg(index + 1).arg(settings.getLoopsCount()));

        if (settings.getFlusingCacheState() && !flushPageCache()) {
            setRunning(false);
            return;
        }

        m_process = new QProcess();
        m_process->start("fio", QStringList()
                         << "--output-format=json"
                         << "--ioengine=libaio"
                         << "--direct=1"
                         << "--randrepeat=0"
                         << "--refill_buffers"
                         << "--end_fsync=1"
                         << QStringLiteral("--rwmixread=%1").arg(settings.getRandomReadPercentage())
                         << QStringLiteral("--filename=%1").arg(getBenchmarkFile())
                         << QStringLiteral("--name=%1").arg(rw)
                         << QStringLiteral("--size=%1m").arg(settings.getFileSize())
                         << QStringLiteral("--zero_buffers=%1").arg(settings.getBenchmarkTestData() == Global::BenchmarkTestData::Zeros)
                         << QStringLiteral("--bs=%1k").arg(blockSize)
                         << QStringLiteral("--runtime=%1").arg(settings.getMeasuringTime())
                         << QStringLiteral("--rw=%1").arg(rw)
                         << QStringLiteral("--iodepth=%1").arg(queueDepth)
                         << QStringLiteral("--numjobs=%1").arg(threads));

        QEventLoop loop;

        auto exitLoop = [&] (bool success, QString output, QString errorOutput) {
            loop.exit();

            if (!success) {
                setRunning(false);
            }

            if (m_running) {
                index++;

                auto result = parseResult(output, errorOutput);

                switch (settings.getPerformanceProfile())
                {
                    case Global::PerformanceProfile::Default:
                        totalRead  += result.read;
                        totalWrite += result.write;
                    break;
                    case Global::PerformanceProfile::Peak:
                    case Global::PerformanceProfile::RealWorld:
                    case Global::PerformanceProfile::Demo:
                        totalRead.updateWithBetterValues(result.read);
                        totalWrite.updateWithBetterValues(result.write);
                    break;
                }
            }

            if (rw.contains("read")) {
                sendResult(totalRead, index);
            }
            else if (rw.contains("write")) {
                sendResult(totalWrite, index);
            }
            else if (rw.contains("rw")) {
                float p = settings.getRandomReadPercentage();
                sendResult((totalRead * p + totalWrite * (100.f - p)) / 100.f, index);
            }
        };

        auto conn = QObject::connect(m_process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), [=] (int exitCode, QProcess::ExitStatus exitStatus) {
            exitLoop(exitStatus == QProcess::NormalExit, QString(m_process->readAllStandardOutput()), QString(m_process->readAllStandardError()));
        });

        loop.exec();

        QObject::disconnect(conn);
    }
}

void Benchmark::sendResult(const Benchmark::PerformanceResult &result, const int index)
{
    const AppSettings settings;
    if (settings.getPerformanceProfile() == Global::PerformanceProfile::Default) {
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

Benchmark::ParsedJob Benchmark::parseResult(const QString &output, const QString &errorOutput)
{
    QJsonDocument jsonResponse = QJsonDocument::fromJson(output.toUtf8());
    QJsonObject jsonObject = jsonResponse.object();
    QJsonArray jobs = jsonObject["jobs"].toArray();

    ParsedJob parsedJob {{0, 0, 0}, {0, 0, 0}};

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
                emit failed(errorOutput/*.mid(errorOutput.simplified().lastIndexOf("=") + 1)*/);
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
        if (!m_process) return;

        if (m_process->state() == QProcess::Running || m_process->state() == QProcess::Starting) {
            m_process->terminate();
            m_process->waitForFinished(-1);
        }

        delete m_process;
    }

    emit runningStateChanged(state);
}

bool Benchmark::isRunning()
{
    return m_running;
}

void Benchmark::runBenchmark(QList<QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>>> tests)
{
    setRunning(true);

    const AppSettings settings;

    QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>> item;

    QMutableListIterator<QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>>> iter(tests);
    // Set to 0 all the progressbars for current tests
    while (iter.hasNext()) {
        item = iter.next();
        if (item.first.second == Global::BenchmarkIOReadWrite::Read && settings.getBenchmarkMode() == Global::BenchmarkMode::WriteMix) { iter.remove(); continue; }
        if (item.first.second == Global::BenchmarkIOReadWrite::Write && settings.getBenchmarkMode() == Global::BenchmarkMode::ReadMix) { iter.remove(); continue; }
        auto progressBars = item.second;
        for (auto obj : progressBars) {
            emit resultReady(obj, PerformanceResult());
        }
    }

    iter.toFront();

    // If there are no tests in the queue
    if (!iter.hasNext()) {
        setRunning(false);
        return;
    }

    emit benchmarkStatusUpdate(tr("Preparing..."));

    if (!prepareFile(getBenchmarkFile(), settings.getFileSize())) {
        setRunning(false);
        return;
    }

    while (iter.hasNext() && m_running) {
        item = iter.next();

        m_progressBars = item.second;

        Global::BenchmarkParams params = settings.getBenchmarkParams(item.first.first, settings.getPerformanceProfile());

        switch (item.first.second)
        {
        case Global::BenchmarkIOReadWrite::Read:
            if (params.Pattern == Global::BenchmarkIOPattern::SEQ) {
                startTest(params.BlockSize, params.Queues, params.Threads,
                          Global::getRWSequentialRead(), tr("Sequential Read %1/%2"));
            }
            else {
                startTest(params.BlockSize, params.Queues, params.Threads,
                          Global::getRWRandomRead(), tr("Random Read %1/%2"));
            }
            break;
        case Global::BenchmarkIOReadWrite::Write:
            if (params.Pattern == Global::BenchmarkIOPattern::SEQ) {
                startTest(params.BlockSize, params.Queues, params.Threads,
                          Global::getRWSequentialWrite(), tr("Sequential Write %1/%2"));
            }
            else {
                startTest(params.BlockSize, params.Queues, params.Threads,
                          Global::getRWRandomWrite(), tr("Random Write %1/%2"));
            }
            break;
        case Global::BenchmarkIOReadWrite::Mix:
            if (params.Pattern == Global::BenchmarkIOPattern::SEQ) {
                startTest(params.BlockSize, params.Queues, params.Threads,
                          Global::getRWSequentialMix(), tr("Sequential Mix %1/%2"));
            }
            else {
                startTest(params.BlockSize, params.Queues, params.Threads,
                          Global::getRWRandomMix(), tr("Random Mix %1/%2"));
            }
            break;
        }

        if (iter.hasNext()) {
            for (int i = 0, intervalTime = settings.getIntervalTime(); i < intervalTime && m_running; i++) {
                emit benchmarkStatusUpdate(tr("Interval Time %1/%2 sec").arg(i).arg(intervalTime));
                QEventLoop loop;
                QTimer::singleShot(1000, &loop, SLOT(quit()));
                loop.exec();
            }
        }
    }

    QFile(getBenchmarkFile()).remove();

    setRunning(false);
    emit finished(); // Only needed when closing the app during a running benchmarking
}

bool Benchmark::listStorages()
{
    QVector<Global::Storage> storages;

    foreach (const QStorageInfo &storage, QStorageInfo::mountedVolumes()) {
        if (storage.isValid() && storage.isReady() && !storage.isReadOnly()) {
            if (storage.device().indexOf("/dev") != -1) {
                storages.append({ storage.rootPath(), storage.bytesTotal(), storage.bytesTotal() - storage.bytesAvailable() });
            }
        }
    }

    emit mountPointsListReady(storages);

    return true;
}

bool Benchmark::flushPageCache()
{
    QFile file("/proc/sys/vm/drop_caches");

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write("1");
        file.close();
    }
    else {
        emit failed(file.errorString());
        return false;
    }

    return true;
}

bool Benchmark::prepareFile(const QString &benchmarkFile, int fileSize)
{
    bool prepared = true;

    m_process = new QProcess();
    m_process->start("fio", QStringList()
                     << "--output-format=json"
                     << "--create_only=1"
                     << QStringLiteral("--filename=%1").arg(benchmarkFile)
                     << QStringLiteral("--size=%1m").arg(fileSize)
                     << QStringLiteral("--zero_buffers=%1").arg(AppSettings().getBenchmarkTestData() == Global::BenchmarkTestData::Zeros)
                     << QStringLiteral("--name=prepare"));

    QEventLoop loop;

    auto exitLoop = [&] (bool success, QString output, QString errorOutput) {
        loop.exit();

        if (!success) {
            prepared = false;
            if (!errorOutput.isEmpty()) {
                setRunning(false);
                emit failed(errorOutput);
            }
        }

        if (m_running) {
            parseResult(output, errorOutput);
        }
    };

    auto conn = QObject::connect(m_process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), [=] (int exitCode, QProcess::ExitStatus exitStatus) {
        exitLoop(exitStatus == QProcess::NormalExit, QString(m_process->readAllStandardOutput()), QString(m_process->readAllStandardError()));
    });

    loop.exec();

    QObject::disconnect(conn);

    return prepared;
}
