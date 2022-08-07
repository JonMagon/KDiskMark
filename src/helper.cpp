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


void Helper::exit()
{
    m_loop->exit();

    QDBusConnection::systemBus().unregisterObject(QStringLiteral("/Helper"));
    QDBusConnection::systemBus().unregisterService(QStringLiteral("dev.jonmagon.kdiskmark.helperinterface"));
}

ActionReply Helper::dropcache(const QVariantMap& args)
{
    if (args["check"].toBool()) {
        return {};
    }

    ActionReply reply;

    QFile file("/proc/sys/vm/drop_caches");

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        reply = ActionReply::HelperErrorReply();
        reply.setErrorDescription(file.errorString());
        return reply;
    }

    file.write("1");

    file.close();

    return reply;
}

KAUTH_HELPER_MAIN("dev.jonmagon.kdiskmark.helper", Helper)
