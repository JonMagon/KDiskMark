#include "benchmark.h"

#include <KAuth>

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
    emit benchmarkStatusUpdate(tr("Preparing..."));

    const AppSettings settings;

    if (!prepareFile(getBenchmarkFile(), settings.getFileSize(), rw)) {
        setRunning(false);
        return;
    }

    PerformanceResult totalRead { 0, 0, 0 }, totalWrite { 0, 0, 0 };

    unsigned int index = 0;

    for (int i = 0; i < settings.getLoopsCount(); i++) {
        if (!m_running) break;

        emit benchmarkStatusUpdate(statusMessage.arg(index + 1).arg(settings.getLoopsCount()));

        if (settings.getFlusingCacheState() && !flushPageCache()) {
            setRunning(false);
            return;
        }

        auto interface = helperInterface();
        if (!interface) {
            setRunning(false);
            emit failed("Inteface is null");
            return;
        }

        QDBusPendingCall pcall = interface->startTest(getBenchmarkFile(),
                                                      settings.getMeasuringTime(),
                                                      settings.getFileSize(),
                                                      settings.getRandomReadPercentage(),
                                                      settings.getBenchmarkTestData() == Global::BenchmarkTestData::Zeros,
                                                      blockSize, queueDepth, threads, rw);
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
        auto interface = helperInterface();
        if (interface)
            dbusWaitForFinish(interface->stopCurrentTask());
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

    auto interface = helperInterface();
    if (interface)
        dbusWaitForFinish(interface->removeFile(getBenchmarkFile()));

    setRunning(false);
    emit finished();
}

DevJonmagonKdiskmarkHelperInterface* Benchmark::helperInterface()
{
    if (!QDBusConnection::systemBus().isConnected()) {
        qCritical() << QDBusConnection::systemBus().lastError().message();
        return nullptr;
    }

    auto *interface = new dev::jonmagon::kdiskmark::helper(QStringLiteral("dev.jonmagon.kdiskmark.helper"),
                QStringLiteral("/Helper"), QDBusConnection::systemBus(), this);
    interface->setTimeout(10 * 24 * 3600 * 1000); // 10 days

    return interface;
}

bool Benchmark::startHelper()
{
    if (!QDBusConnection::systemBus().isConnected()) {
        qWarning() << QDBusConnection::systemBus().lastError().message();
        return false;
    }

    QDBusInterface interface(QStringLiteral("dev.jonmagon.kdiskmark.helperinterface"), QStringLiteral("/Helper"), QStringLiteral("dev.jonmagon.kdiskmark.helper"), QDBusConnection::systemBus());
    if (interface.isValid()) {
        qWarning() << "Dbus interface is already in use.";
        return false;
    }

    m_thread = new DBusThread;
    m_thread->start();

    KAuth::Action action = KAuth::Action(QStringLiteral("dev.jonmagon.kdiskmark.helper.init"));
    action.setHelperId(QStringLiteral("dev.jonmagon.kdiskmark.helper"));
    action.setTimeout(10 * 24 * 3600 * 1000); // 10 days

    QVariantMap arguments;
    action.setArguments(arguments);
    KAuth::ExecuteJob *job = action.execute();
    job->start();

    QEventLoop loop;
    auto exitLoop = [&] () { loop.exit(); };
    auto conn = QObject::connect(job, &KAuth::ExecuteJob::newData, exitLoop);
    QObject::connect(job, &KJob::finished, [=] () { if (job->error()) exitLoop(); } );
    loop.exec();
    QObject::disconnect(conn);

    return !job->error();
}

bool Benchmark::listStorages()
{
    auto interface = helperInterface();
    if (!interface) return false;

    QDBusPendingCall pcall = interface->listStorages();

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pcall, this);
    QEventLoop loop;

    auto exitLoop = [&] (QDBusPendingCallWatcher *watcher) {
        loop.exit();

        if (watcher->isError())
            qWarning() << watcher->error();
        else {
            QDBusPendingReply<QVariantMap> reply = *watcher;

            QVariantMap replyContent = reply.value();

            QVector<Global::Storage> storages;

            for (auto pathStorage : replyContent.keys()) {
                QDBusVariant dbusVariant = qvariant_cast<QDBusVariant>(replyContent.value(pathStorage));
                QVector<qlonglong> storageStats;
                dbusVariant.variant().value<QDBusArgument>() >> storageStats;
                storages.append({ pathStorage, storageStats[0], storageStats[0] - storageStats[1] });
            }

            emit mountPointsListReady(storages);
        }
    };

    connect(watcher, &QDBusPendingCallWatcher::finished, exitLoop);
    loop.exec();

    return true;
}

bool Benchmark::flushPageCache()
{
    bool flushed = true;

    auto interface = helperInterface();
    if (!interface) return false;

    QDBusPendingCall pcall = interface->flushPageCache();

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pcall, this);
    QEventLoop loop;

    auto exitLoop = [&] (QDBusPendingCallWatcher *watcher) {
        loop.exit();

        if (watcher->isError())
            qWarning() << watcher->error();
        else {
            QDBusPendingReply<QVariantMap> reply = *watcher;

            QVariantMap replyContent = reply.value();
            if (!replyContent[QStringLiteral("success")].toBool()) {
                flushed = false;
                QString error = replyContent[QStringLiteral("error")].toString();
                if (!error.isEmpty()) {
                    setRunning(false);
                    emit failed(error);
                }
            }
        }
    };

    connect(watcher, &QDBusPendingCallWatcher::finished, exitLoop);
    loop.exec();

    return flushed;
}

bool Benchmark::prepareFile(const QString &benchmarkFile, int fileSize, const QString &rw)
{
    bool flushed = true;

    auto interface = helperInterface();
    if (!interface) return false;

    QDBusPendingCall pcall = interface->prepareFile(benchmarkFile, fileSize, AppSettings().getBenchmarkTestData() == Global::BenchmarkTestData::Zeros);

    QEventLoop loop;

    auto exitLoop = [&] (bool success, QString output, QString errorOutput) {
        loop.exit();

        if (!success) {
            flushed = false;
            if (!errorOutput.isEmpty()) {
                setRunning(false);
                emit failed(errorOutput);
            }
        }

        if (m_running) {
            parseResult(output, errorOutput);
        }
    };

    auto conn = QObject::connect(interface, &DevJonmagonKdiskmarkHelperInterface::taskFinished, exitLoop);

    loop.exec();

    QObject::disconnect(conn);

    return flushed;
}

void Benchmark::dbusWaitForFinish(QDBusPendingCall pcall)
{
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pcall, this);
    QEventLoop loop;
    connect(watcher, &QDBusPendingCallWatcher::finished, [&] (QDBusPendingCallWatcher *watcher) { loop.exit(); });
    loop.exec();
}

void Benchmark::stopHelper()
{
    auto *interface = new dev::jonmagon::kdiskmark::helper(QStringLiteral("dev.jonmagon.kdiskmark.helper"),
                                                           QStringLiteral("/Helper"), QDBusConnection::systemBus());
    interface->exit();
}

void DBusThread::run()
{
    if (!QDBusConnection::systemBus().registerService(QStringLiteral("dev.jonmagon.kdiskmark.applicationinterface")) ||
        !QDBusConnection::systemBus().registerObject(QStringLiteral("/Application"), this, QDBusConnection::ExportAllSlots)) {
        qWarning() << QDBusConnection::systemBus().lastError().message();
        return;
    }

    QEventLoop loop;
    loop.exec();
}
