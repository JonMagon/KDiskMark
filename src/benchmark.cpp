#include "benchmark.h"

#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>
#include <QFile>
#include <QEventLoop>

#include "appsettings.h"
#include "global.h"

#include "helper_interface.h"

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

Benchmark::~Benchmark() {}

QString Benchmark::getFIOVersion()
{
    return m_FIOVersion;
}

bool Benchmark::isFIODetected()
{
    return m_FIOVersion.indexOf("fio-") == 0;
}

void Benchmark::startTest(int blockSize, int queueDepth, int threads, const QString &rw, const QString &statusMessage)
{
    emit benchmarkStatusUpdate(tr("Preparing..."));

    if (!prepareFile(m_settings->getBenchmarkFile(), m_settings->getFileSize(), rw)) {
        setRunning(false);
        return;
    }

    PerformanceResult totalRead { 0, 0, 0 }, totalWrite { 0, 0, 0 };

    unsigned int index = 0;

    for (int i = 0; i < m_settings->getLoopsCount(); i++) {
        if (!m_running) break;

        emit benchmarkStatusUpdate(statusMessage.arg(index).arg(m_settings->getLoopsCount()));

        if (m_settings->shouldFlushCache() && !flushPageCache()) {
            setRunning(false);
            return;
        }


if (!m_helperStarted)
    if (!startHelper()) {
        qWarning("Could not obtain administrator privileges.");
        exit(0); /// TEST
    }

auto interface = helperInterface();
if (!interface)
    exit(0); /// TEST

        QDBusPendingCall pcall = interface->startTest(m_settings->getBenchmarkFile(),
                                                      m_settings->getMeasuringTime(),
                                                      m_settings->getFileSize(),
                                                      m_settings->getRandomReadPercentage(),
                                                      blockSize, queueDepth, threads, rw);

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
                    setRunning(false);
                }

                if (m_running) {
                    index++;

                    auto result = parseResult(QString::fromLocal8Bit(replyContent[QStringLiteral("output")].toByteArray()),
                            QString::fromLocal8Bit(replyContent[QStringLiteral("errorOutput")].toByteArray()));

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
        };

        connect(watcher, &QDBusPendingCallWatcher::finished, exitLoop);
        loop.exec();
    }
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
        /*for (auto &process : m_processes) {
            if (process->state() == QProcess::Running || process->state() == QProcess::Starting) {
                kill(process->processId(), SIGINT);
            }
        }*/
    }

    emit runningStateChanged(state);
}

bool Benchmark::isRunning()
{
    return m_running;
}

void Benchmark::runBenchmark(QList<QPair<Benchmark::Type, QVector<QProgressBar*>>> tests)
{
    setRunning(true);

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
            startTest(params.BlockSize, params.Queues, params.Threads,
                      Global::getRWSequentialRead(), tr("Sequential Read %1/%2"));
            break;
        case SEQ_1_Write:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_1);
            startTest(params.BlockSize, params.Queues, params.Threads,
                      Global::getRWSequentialWrite(), tr("Sequential Write %1/%2"));
            break;
        case SEQ_1_Mix:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_1);
            startTest(params.BlockSize, params.Queues, params.Threads,
                      Global::getRWSequentialMix(), tr("Sequential Mix %1/%2"));
            break;
        case SEQ_2_Read:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_2);
            startTest(params.BlockSize, params.Queues,params.Threads,
                      Global::getRWSequentialRead(), tr("Sequential Read %1/%2"));
            break;
        case SEQ_2_Write:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_2);
            startTest(params.BlockSize, params.Queues, params.Threads,
                      Global::getRWSequentialWrite(), tr("Sequential Write %1/%2"));
            break;
        case SEQ_2_Mix:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_2);
            startTest(params.BlockSize, params.Queues, params.Threads,
                      Global::getRWSequentialMix(), tr("Sequential Mix %1/%2"));
            break;
        case RND_1_Read:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_1);
            startTest(params.BlockSize, params.Queues, params.Threads,
                      Global::getRWRandomRead(), tr("Random Read %1/%2"));
            break;
        case RND_1_Write:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_1);
            startTest(params.BlockSize, params.Queues, params.Threads,
                      Global::getRWRandomWrite(), tr("Random Write %1/%2"));
            break;
        case RND_1_Mix:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_1);
            startTest(params.BlockSize, params.Queues, params.Threads,
                      Global::getRWSequentialMix(), tr("Random Mix %1/%2"));
            break;
        case RND_2_Read:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_2);
            startTest(params.BlockSize, params.Queues, params.Threads,
                      Global::getRWRandomRead(), tr("Random Read %1/%2"));
            break;
        case RND_2_Write:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_2);
            startTest(params.BlockSize, params.Queues, params.Threads,
                      Global::getRWRandomWrite(), tr("Random Write %1/%2"));
            break;
        case RND_2_Mix:
            params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_2);
            startTest(params.BlockSize, params.Queues, params.Threads,
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

DevJonmagonKdiskmarkHelperInterface* Benchmark::helperInterface()
{
    if (!QDBusConnection::systemBus().isConnected()) {
        qWarning() << QDBusConnection::systemBus().lastError().message();
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
    m_job = action.execute();
    m_job->start();

    QEventLoop loop;
    auto exitLoop = [&] () { loop.exit(); };
    auto conn = QObject::connect(m_job, &KAuth::ExecuteJob::newData, exitLoop);
    QObject::connect(m_job, &KJob::finished, [=] () { if (m_job->error()) exitLoop(); } );
    loop.exec();
    QObject::disconnect(conn);

    m_helperStarted = true;
    return true;
}

bool Benchmark::listStorages()
{
if (!m_helperStarted)
    if (!startHelper()) {
        qWarning("Could not obtain administrator privileges.");
        return false;
    }

auto interface = helperInterface();
if (!interface)
    return false;

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

            storages.clear();

            for (auto pathStorage : replyContent.keys()) {
                QDBusVariant dbusVariant = qvariant_cast<QDBusVariant>(replyContent.value(pathStorage));
                QVector<qlonglong> storageStats;
                dbusVariant.variant().value<QDBusArgument>() >> storageStats;
                storages.append({ pathStorage, storageStats[0], storageStats[0] - storageStats[1] });
            }
        }
    };

    connect(watcher, &QDBusPendingCallWatcher::finished, exitLoop);
    loop.exec();

    return true;
}

// Combine two next ?

bool Benchmark::flushPageCache()
{
if (!m_helperStarted)
    if (!startHelper()) {
        qWarning("Could not obtain administrator privileges.");
        return false;
    }

auto interface = helperInterface();
if (!interface)
    return false;

    bool flushed = true;

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
                QString error = replyContent[QStringLiteral("success")].toString();
                if (!error.isEmpty())
                    emit failed(error);
            }
        }
    };

    connect(watcher, &QDBusPendingCallWatcher::finished, exitLoop);
    loop.exec();

    return flushed;
}

bool Benchmark::prepareFile(const QString &benchmarkFile, int fileSize, const QString &rw)
{
if (!m_helperStarted)
    if (!startHelper()) {
        qWarning("Could not obtain administrator privileges.");
        return false;
    }

auto interface = helperInterface();
if (!interface)
    return false;

    bool flushed = true;

    QDBusPendingCall pcall = interface->prepareFile(benchmarkFile, fileSize, rw);

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
                QString error = replyContent[QStringLiteral("success")].toString();
                if (!error.isEmpty())
                    emit failed(error);
            }
        }
    };

    connect(watcher, &QDBusPendingCallWatcher::finished, exitLoop);
    loop.exec();

    return flushed;
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
