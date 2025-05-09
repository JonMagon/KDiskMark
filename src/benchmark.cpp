#include "benchmark.h"

#include <QFile>
#include <QEventLoop>
#include <QTimer>
#include <QStorageInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QBuffer>

#include "global.h"

#include <QDebug>

#include <QStandardPaths>

Benchmark::Benchmark()
#ifdef APPIMAGE_EDITION
    : m_localServer(new QLocalServer), m_nextBlockSize(0)
#endif
{
    m_running = false;

    QProcess process;
    process.start("fio", {"--version"});
    process.waitForFinished();

    m_FIOVersion = process.readAllStandardOutput().simplified();

    process.close();

#ifdef APPIMAGE_EDITION
    connect(m_localServer, &QLocalServer::newConnection, this, &Benchmark::helperConnected);
#endif
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

#ifdef APPIMAGE_EDITION
        if (settings.getFlusingCacheState()) {
            flushPageCache();
        }
#endif

        if (m_benchmarkFile->fileName().isNull() || !QFile(m_benchmarkFile->fileName()).exists()) {
            emit failed("The benchmark file was not pre-created.");
            setRunning(false);
            return;
        }

        m_process = new QProcess();
        m_process->start("fio", QStringList()
                         << QStringLiteral("--output-format=json")
                         << QStringLiteral("--ioengine=libaio")
                         << QStringLiteral("--randrepeat=0")
                         << QStringLiteral("--refill_buffers")
                         << QStringLiteral("--end_fsync=1")
                         << QStringLiteral("--direct=%1").arg(settings.getCacheBypassState())
                         << QStringLiteral("--rwmixread=%1").arg(settings.getRandomReadPercentage())
                         << QStringLiteral("--filename=%1").arg(m_benchmarkFile->fileName())
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

    if (!m_running) {
#ifdef APPIMAGE_EDITION
        sendMessageToSocket(m_helperSocket, "HALT");
        m_localServer->close();
        QLocalServer::removeServer(m_localServer->serverName());
#endif

        if (m_process) {
            if (m_process->state() == QProcess::Running || m_process->state() == QProcess::Starting) {
                m_process->terminate();

                QEventLoop loop;

                auto conn = QObject::connect(m_process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), [&] (int exitCode, QProcess::ExitStatus exitStatus) {
                    loop.exit();
                });

                loop.exec();

                QObject::disconnect(conn);
            }

            delete m_process;
            m_process = nullptr;
        }
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

#ifdef APPIMAGE_EDITION
    QUuid uuid = QUuid::createUuid();
    if (!m_localServer->listen("kdiskmark" + uuid.toString(QUuid::StringFormat::WithoutBraces)))
    {
        qCritical() << "Server error:" << m_localServer->errorString();
        m_localServer->close();
        return;
    }
#endif

#ifdef APPIMAGE_EDITION
    if (settings.getFlusingCacheState() && !initHelper(uuid.toString(QUuid::StringFormat::WithoutBraces))) {
        setRunning(false);
        return;
    }
#endif

    if (!prepareBenchmarkFile(getBenchmarkFile(), settings.getFileSize())) {
        setRunning(false);
    }

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

    if (!m_benchmarkFile->fileName().isNull() || QFile(m_benchmarkFile->fileName()).exists()) {
        m_benchmarkFile->close();
        m_benchmarkFile->remove();
        delete m_benchmarkFile;
        m_benchmarkFile = nullptr;
    }

    setRunning(false);
    emit finished(); // Only needed when closing the app during a running benchmarking
}

#ifdef APPIMAGE_EDITION
bool Benchmark::initHelper(const QString& id)
{
    QString appImagePath = QProcessEnvironment::systemEnvironment().value(QStringLiteral("APPIMAGE"));

    if (appImagePath.isEmpty()) {
        emit failed("Failed to get AppImage path. The helper could not be started.");
        return false;
    }

    QString pkexec = QStandardPaths::findExecutable("pkexec"),
            kdesu = QStandardPaths::findExecutable("kdesu");

    if (pkexec.isEmpty() && kdesu.isEmpty()) {
        emit failed("No graphical sudo wrapper found. Please install pkexec or kdesu.");
        return false;
    }

    m_process = new QProcess();
    if (!pkexec.isEmpty()) {
        m_process->start(pkexec, {appImagePath, "--helper", id});
    }
    else {
        m_process->start(kdesu, {"--noignorebutton", "-n", "-c", QStringLiteral("%1 --helper %2").arg(appImagePath).arg(id)});
    }

    bool helperState = true;

    QEventLoop loop;
    auto connSocket = QObject::connect(m_localServer, &QLocalServer::newConnection, [&]() { loop.exit(); });
    // If the process ended before newConnection was called, something went wrong
    auto connProcess = QObject::connect(m_process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                [&] (int exitCode, QProcess::ExitStatus exitStatus) {
        if (isRunning()) {
            emit failed(tr("Could not obtain administrator privileges."));
        }
        helperState = false;
        loop.exit();
    });
    loop.exec();

    QObject::disconnect(connSocket);
    QObject::disconnect(connProcess);

    return helperState;
}

void Benchmark::sendMessageToSocket(QLocalSocket* localSocket, const QString& message)
{
    if (!localSocket) return;

    QByteArray array;
    QDataStream out(&array, QIODevice::WriteOnly);
    out << quint16(0) << message;
    out.device()->seek(0);
    out << quint16(array.size() - sizeof(quint16));
    localSocket->write(array);

    qInfo() << "S->" << message;
}

void Benchmark::flushPageCache()
{
    if (m_benchmarkFile->fileName().isNull() || !QFile(m_benchmarkFile->fileName()).exists()) {
        emit failed("A benchmark file must first be created.");
        setRunning(false);
        return;
    }

    sendMessageToSocket(m_helperSocket, "FLUSHCACHE");

    QEventLoop loop;
    auto connSocket = QObject::connect(m_helperSocket, &QLocalSocket::readyRead, [&]() { loop.exit(); });
    auto connHelper = QObject::connect(m_helperSocket, &QLocalSocket::disconnected, [&]() { loop.exit(); });
    loop.exec();

    QObject::disconnect(connSocket);
    QObject::disconnect(connHelper);
}

void Benchmark::helperConnected()
{
    QLocalSocket* localSocket = m_localServer->nextPendingConnection();

    if (m_helperSocket) {
        localSocket->abort();
        qInfo() << "New helper rejected";
        return;
    }
    else {
        qInfo() << "New helper connected";
        m_helperSocket = localSocket;
        connect(localSocket, &QLocalSocket::disconnected, [this]() {
            qInfo() << "Helper disconnected";
            m_helperSocket = nullptr;
            setRunning(false);
        });
        connect(localSocket, &QLocalSocket::readyRead, this, &Benchmark::readHelper);
    }
}

void Benchmark::readHelper()
{
    QLocalSocket* localSocket = (QLocalSocket*)sender();
    QDataStream in(localSocket);
    for (;;)
    {
        if (!m_nextBlockSize)
        {
            if (localSocket->bytesAvailable() < (int)sizeof(quint16))
                break;
            in >> m_nextBlockSize;
        }

        QString message;
        in >> message;

        qInfo() << "S<-" << message;

        if (message.startsWith("ERROR")) {
            emit failed(message.mid(6));
            setRunning(false);
        }

        m_nextBlockSize = 0;
    }
}
#endif

bool Benchmark::testFilePath(const QString &benchmarkPath)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    if (QFileInfo(benchmarkPath).isSymbolicLink()) {
#else
    // detects *.lnk on Windows, but there's not Windows version, whatever
    if (QFileInfo(benchmarkPath).isSymLink()) {
#endif
        qWarning("The path should not be symbolic link.");
        return false;
    }

    // Actually superfluous because of above, makes the check more obvious
    // Just in case something changes in the backend
    if (benchmarkPath.startsWith("/dev")) {
        qWarning("Cannot specify a raw device.");
        return false;
    }

    return true;
}

bool Benchmark::prepareBenchmarkFile(const QString &benchmarkPath, int fileSize)
{
    bool prepared = true;

    if (m_benchmarkFile && !m_benchmarkFile->fileName().isNull()) {
        emit failed("A new benchmark session should be started.");
        return false;
    }

    if (!testFilePath(benchmarkPath)) {
        emit failed("The path to the file is incorrect.");
        return false;
    }

    m_benchmarkFile = new QTemporaryFile();
    m_benchmarkFile->setFileTemplate(QStringLiteral("%1/%2").arg(benchmarkPath).arg("kdiskmark-XXXXXX.tmp"));

    if (!m_benchmarkFile->open()) {
        emit failed("An error occurred while creating the benchmark file.");
        return false;
    }

    m_process = new QProcess();
    m_process->start("fio", QStringList()
                     << QStringLiteral("--output-format=json")
                     << QStringLiteral("--create_only=1")
                     << QStringLiteral("--filename=%1").arg(m_benchmarkFile->fileName())
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
