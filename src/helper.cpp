#include "helper.h"

#include <QCoreApplication>
#include <QtDBus>
#include <QFile>

ActionReply Helper::init(const QVariantMap& args)
{
    Q_UNUSED(args)

    ActionReply reply;

    if (!QDBusConnection::systemBus().isConnected() || !QDBusConnection::systemBus().registerService(QStringLiteral("dev.jonmagon.kdiskmark.helperinterface")) ||
        !QDBusConnection::systemBus().registerObject(QStringLiteral("/Helper"), this, QDBusConnection::ExportAllSlots)) {
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

QVariantMap Helper::prepareFile(const QString &benchmarkFile, int fileSize, const QString &rw)
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    QVariantMap reply;

   // check benchmarkFile is file

    QProcess process;
    process.start("fio", QStringList()
                  << "--create_only=1"
                  << QStringLiteral("--filename=%1").arg(benchmarkFile)
                  << QStringLiteral("--size=%1m").arg(fileSize)
                  << QStringLiteral("--name=%1").arg(rw));

    process.waitForFinished(-1);

    reply[QStringLiteral("success")] = process.exitStatus() == QProcess::NormalExit;
    reply[QStringLiteral("output")] = process.readAllStandardOutput();
    reply[QStringLiteral("errorOutput")] = process.readAllStandardError();

    return reply;
}

QVariantMap Helper::startTest(const QString &benchmarkFile, int measuringTime, int fileSize, int randomReadPercentage,
                              int blockSize, int queueDepth, int threads, const QString &rw)
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    QVariantMap reply;

    // check benchmarkFile is file

    QProcess process;
    process.start("fio", QStringList()
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
                  << QStringLiteral("--bs=%1k").arg(blockSize)
                  << QStringLiteral("--runtime=%1").arg(measuringTime)
                  << QStringLiteral("--rw=%1").arg(rw)
                  << QStringLiteral("--iodepth=%1").arg(queueDepth)
                  << QStringLiteral("--numjobs=%1").arg(threads));

    process.waitForFinished(-1);

    reply[QStringLiteral("success")] = process.exitStatus() == QProcess::NormalExit;
    reply[QStringLiteral("output")] = process.readAllStandardOutput();
    reply[QStringLiteral("errorOutput")] = process.readAllStandardError();

    return reply;
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

void Helper::exit()
{
    m_loop->exit();

    QDBusConnection::systemBus().unregisterObject(QStringLiteral("/Helper"));
    QDBusConnection::systemBus().unregisterService(QStringLiteral("dev.jonmagon.kdiskmark.helperinterface"));
}

KAUTH_HELPER_MAIN("dev.jonmagon.kdiskmark.helper", Helper)
