#include <KAuth>

#include <QEventLoop>

#include <memory>

using namespace KAuth;

class Helper : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "dev.jonmagon.kdiskmark.helper")

public Q_SLOTS:
    ActionReply init(const QVariantMap& args);
    Q_SCRIPTABLE QVariantMap listStorages();
    Q_SCRIPTABLE QVariantMap prepareFile(const QString &benchmarkFile, int fileSize, const QString &rw);
    Q_SCRIPTABLE QVariantMap startTest(const QString &benchmarkFile, int measuringTime, int fileSize, int randomReadPercentage,
                                       int blockSize, int queueDepth, int threads,  const QString &rw);
    Q_SCRIPTABLE QVariantMap flushPageCache();
    Q_SCRIPTABLE void exit();

private:
    std::unique_ptr<QEventLoop> m_loop;
};
