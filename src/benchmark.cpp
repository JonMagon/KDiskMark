#include "benchmark.h"

#include "global.h"

#include "helper_interface.h"

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
    return m_dir;
}

void Benchmark::startTest(int blockSize, int queueDepth, int threads, const QString &rw, const QString &statusMessage)
{
    const AppSettings settings;

    PerformanceResult totalRead { 0, 0, 0 }, totalWrite { 0, 0, 0 };

    unsigned int index = 0;

    for (int i = 0; i < settings.getLoopsCount(); i++) {
        if (!m_running) break;

        emit benchmarkStatusUpdate(statusMessage.arg(index + 1).arg(settings.getLoopsCount()));

        auto interface = helperInterface();
        if (!interface) {
            setRunning(false);
            emit failed("Helper inteface is null.");
            return;
        }

        if (settings.getFlusingCacheState()) {
            handleDbusPendingCall(interface->flushPageCache());
        }

        if (!isRunning()) return;

        handleDbusPendingCall(interface->startBenchmarkTest(settings.getMeasuringTime(),
                                                            settings.getFileSize(),
                                                            settings.getRandomReadPercentage(),
                                                            settings.getBenchmarkTestData() == Global::BenchmarkTestData::Zeros,
                                                            settings.getCacheBypassState(),
                                                            blockSize, queueDepth, threads, rw));

        if (!isRunning()) return;

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

        auto conn = QObject::connect(interface, &DevJonmagonKdiskmarkHelperInterface::taskFinished, exitLoop);

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
                parsedJob.read.Latency += jobRead["clat_ns"].toObject().value("mean").toDouble() / 1000.0 / jobsCount; // to usec

                QJsonObject jobWrite = job["write"].toObject();
                parsedJob.write.Bandwidth += jobWrite.value("bw").toInt() / 1000.0; // to mb
                parsedJob.write.IOPS += jobWrite.value("iops").toDouble();
                parsedJob.write.Latency += jobWrite["clat_ns"].toObject().value("mean").toDouble() / 1000.0 / jobsCount; // to usec
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

    if (!m_running && m_helperAuthorized) {
        auto interface = helperInterface();
        if (interface) handleDbusPendingCall(interface->stopCurrentTask());
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

    initSession(); if (!isRunning()) return;

    prepareDirectory(getBenchmarkFile()); if (!isRunning()) return;

    prepareFile(getBenchmarkFile(), settings.getFileSize());

    while (iter.hasNext() && isRunning()) {
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

    auto interface = helperInterface();
    if (interface) handleDbusPendingCall(interface->removeBenchmarkFile());

    setRunning(false);

    // Turn off the helper, otherwise it will not be able to perform another test
    // Just sending a message without waiting for a response
    if (interface) interface->endSession();

    emit finished(); // Only needed when closing the app during a running benchmarking
}

DevJonmagonKdiskmarkHelperInterface* Benchmark::helperInterface()
{
    if (!QDBusConnection::systemBus().isConnected()) {
        emit failed(QDBusConnection::systemBus().lastError().message());
        return nullptr;
    }

    auto *interface = new dev::jonmagon::kdiskmark::helper(QStringLiteral("dev.jonmagon.kdiskmark.helperinterface"),
                QStringLiteral("/Helper"), QDBusConnection::systemBus(), this);
    interface->setTimeout(10 * 24 * 3600 * 1000); // 10 days

    return interface;
}

void Benchmark::initSession()
{
    m_helperAuthorized = false;

    auto interface = helperInterface();
    if (interface) handleDbusPendingCall(interface->initSession());

    // Process was not stopped by handleDbusPendingCall, consider that the authorization was successful
    if (isRunning()) m_helperAuthorized = true;
}

void Benchmark::prepareDirectory(const QString &benchmarkFile)
{
    auto interface = helperInterface();
    if (!interface) return;

    QDBusPendingCall pcall = interface->checkCowStatus(benchmarkFile);
    handleDbusPendingCall(pcall);
    if (!isRunning()) return;

    QDBusPendingReply<QVariantMap> reply = pcall;
    if (reply.value()["hasCow"].toBool()) {
        bool createNoCowDir = false;
        auto callback = [&createNoCowDir](bool create) {
            createNoCowDir = create;
        };

        auto conn = QObject::connect(this, &Benchmark::createNoCowDirectoryResponse, callback);
        emit cowCheckRequired();
        QObject::disconnect(conn);

        if (createNoCowDir) {
            pcall = interface->createNoCowDirectory(benchmarkFile);
            handleDbusPendingCall(pcall);
            if (!isRunning()) return;

            reply = pcall;
            this->setDir(reply.value()["path"].toString());
            emit directoryChanged(this->getBenchmarkFile());
        }
    }
}

void Benchmark::prepareFile(const QString &benchmarkFile, int fileSize)
{
    auto interface = helperInterface();
    if (!interface) return;

    handleDbusPendingCall(interface->prepareBenchmarkFile(benchmarkFile, fileSize, AppSettings().getBenchmarkTestData() == Global::BenchmarkTestData::Zeros));

    if (!isRunning()) return;

    QEventLoop loop;

    auto exitLoop = [&] (bool success, QString output, QString errorOutput) {
        loop.exit();

        if (!isRunning()) return;

        if (!success) {
            setRunning(false);
            emit failed(!errorOutput.isEmpty() ? errorOutput : "The benchmark file could not be prepared.");
        }

        parseResult(output, errorOutput);
    };

    auto conn = QObject::connect(interface, &DevJonmagonKdiskmarkHelperInterface::taskFinished, exitLoop);

    loop.exec();

    QObject::disconnect(conn);
}

void Benchmark::handleDbusPendingCall(QDBusPendingCall pcall)
{
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pcall, this);
    QEventLoop loop;
    connect(watcher, &QDBusPendingCallWatcher::finished, [&] (QDBusPendingCallWatcher *watcher) {
        loop.exit();

        if (!isRunning()) return;

        if (watcher->isError()) {
            setRunning(false);
            if (watcher->error().type() == QDBusError::AccessDenied) {
                emit failed(tr("Could not obtain administrator privileges."));
            }
            else {
                emit failed(!watcher->error().message().isEmpty() ? watcher->error().message() : watcher->error().name());
            }
        }
        else {
            QDBusPendingReply<QVariantMap> reply = *watcher;

            QVariantMap replyContent = reply.value();
            if (!replyContent[QStringLiteral("success")].toBool()) {
                setRunning(false);
                QString error = replyContent[QStringLiteral("error")].toString();
                emit failed(!error.isEmpty() ? error : "An error has occurred when executing a DBus task.");
            }
        }
    });

    loop.exec();
}
