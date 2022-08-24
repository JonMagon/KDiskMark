#include "helper.h"

#include <QCoreApplication>
#include <QtDBus>
#include <QFile>

#include <signal.h>

HelperAdaptor::HelperAdaptor(Helper *parent) :
    QDBusAbstractAdaptor(parent)
{
    m_parentHelper = parent;
}

QVariantMap HelperAdaptor::listStorages()
{
    return m_parentHelper->listStorages();
}

void HelperAdaptor::prepareFile(const QString &benchmarkFile, int fileSize, bool fillZeros)
{
    return m_parentHelper->prepareFile(benchmarkFile, fileSize, fillZeros);
}

void HelperAdaptor::startTest(const QString &benchmarkFile, int measuringTime, int fileSize, int randomReadPercentage, bool fillZeros,
                              int blockSize, int queueDepth, int threads, const QString &rw)
{
    m_parentHelper->startTest(benchmarkFile, measuringTime, fileSize, randomReadPercentage, fillZeros, blockSize, queueDepth, threads, rw);
}

QVariantMap HelperAdaptor::flushPageCache()
{
    return m_parentHelper->flushPageCache();
}

bool HelperAdaptor::removeFile(const QString &benchmarkFile)
{
    return m_parentHelper->removeFile(benchmarkFile);
}

void HelperAdaptor::stopCurrentTask()
{
    m_parentHelper->stopCurrentTask();
}

void HelperAdaptor::exit()
{
    m_parentHelper->exit();
}

ActionReply Helper::init(const QVariantMap& args)
{
    Q_UNUSED(args)

    ActionReply reply;

    if (!QDBusConnection::systemBus().isConnected() || !QDBusConnection::systemBus().registerService(QStringLiteral("dev.jonmagon.kdiskmark.helperinterface")) ||
        !QDBusConnection::systemBus().registerObject(QStringLiteral("/Helper"), this)) {
        qWarning() << QDBusConnection::systemBus().lastError().message();
        reply.addData(QStringLiteral("success"), false);

        qApp->quit();

        return reply;
    }

    m_loop = std::make_unique<QEventLoop>();
    HelperSupport::progressStep(QVariantMap());

    auto serviceWatcher =
            new QDBusServiceWatcher(QStringLiteral("dev.jonmagon.kdiskmark.applicationinterface"),
                                    QDBusConnection::systemBus(),
                                    QDBusServiceWatcher::WatchForUnregistration,
                                    this);
    connect(serviceWatcher, &QDBusServiceWatcher::serviceUnregistered,
            [this]() {
        m_loop->exit();
    });

    m_loop->exec();
    reply.addData(QStringLiteral("success"), true);

    qApp->quit();

    return reply;
}

QVariantMap Helper::listStorages()
{
    QVariantMap reply;
    foreach (const QStorageInfo &storage, QStorageInfo::mountedVolumes()) {
        if (storage.isValid() && storage.isReady() && !storage.isReadOnly()) {
            if (storage.device().indexOf("/dev") != -1) {
                reply[storage.rootPath()] =
                        QVariant::fromValue(QDBusVariant(QVariant::fromValue(QVector<qlonglong> { storage.bytesTotal(), storage.bytesAvailable() })));
            }
        }
    }

    return reply;
}

void Helper::prepareFile(const QString &benchmarkFile, int fileSize, bool fillZeros)
{
    testFilePath(benchmarkFile);

    m_process = new QProcess();
    m_process->start("fio", QStringList()
                     << "--output-format=json"
                     << "--create_only=1"
                     << QStringLiteral("--filename=%1").arg(benchmarkFile)
                     << QStringLiteral("--size=%1m").arg(fileSize)
                     << QStringLiteral("--zero_buffers=%1").arg(fillZeros)
                     << QStringLiteral("--name=prepare"));

    connect(m_process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            [=] (int exitCode, QProcess::ExitStatus exitStatus) {
        emit taskFinished(exitStatus == QProcess::NormalExit, QString(m_process->readAllStandardOutput()), QString(m_process->readAllStandardError()));
    });
}

void Helper::startTest(const QString &benchmarkFile, int measuringTime, int fileSize, int randomReadPercentage, bool fillZeros,
                       int blockSize, int queueDepth, int threads, const QString &rw)
{
    testFilePath(benchmarkFile);

    m_process = new QProcess();
    m_process->start("fio", QStringList()
                     << "--output-format=json"
                     << "--ioengine=libaio"
                     << "--direct=1"
                     << "--randrepeat=0"
                     << "--refill_buffers"
                     << "--end_fsync=1"
                     << QStringLiteral("--rwmixread=%1").arg(randomReadPercentage)
                     << QStringLiteral("--filename=%1").arg(benchmarkFile)
                     << QStringLiteral("--name=%1").arg(rw)
                     << QStringLiteral("--size=%1m").arg(fileSize)
                     << QStringLiteral("--zero_buffers=%1").arg(fillZeros)
                     << QStringLiteral("--bs=%1k").arg(blockSize)
                     << QStringLiteral("--runtime=%1").arg(measuringTime)
                     << QStringLiteral("--rw=%1").arg(rw)
                     << QStringLiteral("--iodepth=%1").arg(queueDepth)
                     << QStringLiteral("--numjobs=%1").arg(threads));

    connect(m_process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            [=] (int exitCode, QProcess::ExitStatus exitStatus) {
        emit taskFinished(exitStatus == QProcess::NormalExit, QString(m_process->readAllStandardOutput()), QString(m_process->readAllStandardError()));
    });
}

QVariantMap Helper::flushPageCache()
{
    QVariantMap reply;
    reply[QStringLiteral("success")] = true;

    QFile file("/proc/sys/vm/drop_caches");

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write("1");
        file.close();
    }
    else {
        reply[QStringLiteral("success")] = false;
        reply[QStringLiteral("error")] = file.errorString();
    }

    return reply;
}

void Helper::testFilePath(const QString &benchmarkFile)
{
    if (!benchmarkFile.endsWith("/.kdiskmark.tmp"))
        qFatal("The path must end with /.kdiskmark.tmp");
}

bool Helper::removeFile(const QString &benchmarkFile)
{
    return QFile(benchmarkFile).remove();
}

void Helper::stopCurrentTask()
{
    if (!m_process) return;

    if (m_process->state() == QProcess::Running || m_process->state() == QProcess::Starting) {
        m_process->terminate();
        m_process->waitForFinished(-1);
    }

    delete m_process;
}

void Helper::exit()
{
    m_loop->exit();

    QDBusConnection::systemBus().unregisterObject(QStringLiteral("/Helper"));
    QDBusConnection::systemBus().unregisterService(QStringLiteral("dev.jonmagon.kdiskmark.helperinterface"));
}

KAUTH_HELPER_MAIN("dev.jonmagon.kdiskmark.helper", Helper)
